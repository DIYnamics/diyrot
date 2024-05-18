#ifndef _ADV_ARGUTILS_H_
#define _ADV_ARGUTILS_H_

#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/core/base.hpp>
#include <opencv2/core/core.hpp>
#include <vector>
#include <unordered_map>

#define WINDOW 30

typedef struct {
    std::vector<cv::Point2f> history;
    bool valid;
    cv::Scalar color;
} SinglePointHistory;

/* derivables from SinglePointHistory for recording:
r = cv::norm(point_in_rotframe);
theta = std::atan2(point_in_rotframe.y, point_in_rotframe.x);
-- gradient funcs are most efficient when entire history is passed,
-- but we need the central gradient as soon as its available for drawing the frame at the time
dx = gradient(history.x, t)
dy = gradient(history.y, t)
dr = gradient(r, t)
dtheta = gradient(theta, t)
velocity = cv::norm(dx, dy) -- one norm per point

things that need to be drawn:
-- major issue: ployfit/gradient requires datapoints from future frames, processing loop does not support
velocity -- line from (x, y) to (x, y) + velocity
other line --
totOmega = RPM * 2pi / 60 // angular rotation velocity rad/s
rinert = velocity / totOmega // m/s / rad/s => m / rad
rinertsmooth = polyval(polyfit(rinert, t, 20)): in digipyro, this is a 20-degree fit across the entire video! cannot work for all video sizes. maybe do a 3-degree fit on 20 surrounding data points
    note that the polyfit requires looking at datapoints in a window
dxsmooth = polyval(polyfit(dx, t, *
dysmooth = polyval(polyfit(dy, t, *

draw the velocity line from the tracked xy point to tracked_x+dxsmooth, tracked_y+dysmooth
angle = np.arctan2(dySmooth,dxSmooth) // rad
draw a line from the tracked xy point to tracked_x+(rad*np.sin(angle))), tracked_y-(rad*np.cos(angle)) // this is just the line showing dx, dy smoothed, (?) in the rotating frame
                                                                                                       // you should run the original digipyro code to figure out what exactly this line is supposed to be
*/

typedef struct {
    std::vector<SinglePointHistory> points;
    std::vector<float> t;
    bool valid;
    int point_size;
} PointsHistory;

std::vector<std::string> deserialize(std::string in) {
    const char sep = ',';
    std::vector<std::string> r;
    r.reserve(std::count(in.begin(), in.end(), sep) + 1);
    for (auto p = in.begin();; ++p) {
        auto q = p;
        p = std::find(p, in.end(), sep);
        r.emplace_back(q, p);
        if (p == in.end())
            return r;
    }
}

PointsHistory auto_getpoints(std::string filename, std::string input,
                             cv::Point2f center) {
    auto randColor = [] { return 128+(rand() % static_cast<int>(128)); };

    auto separated_input = deserialize(input);
    double inner_radius = strtod(separated_input[0].c_str(), NULL);
    double outer_radius = strtod(separated_input[1].c_str(), NULL);
    // open video
    auto vid = cv::VideoCapture(filename);

    auto dims = cv::Size(vid.get(cv::CAP_PROP_FRAME_WIDTH),
                         vid.get(cv::CAP_PROP_FRAME_HEIGHT));
    cv::Mat center_mask = cv::Mat::zeros(dims, CV_8UC1);
    cv::circle(center_mask, center, outer_radius, 255, -1);
    cv::bitwise_not(center_mask, center_mask);
    cv::circle(center_mask, center, inner_radius, 255, -1);

    cv::Mat frame;
    if (!vid.read(frame))
        return PointsHistory{.valid = false};

    cv::Mat frame_gray;
    cv::cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
    cv::bitwise_and(frame_gray, 0, frame_gray, center_mask);

    std::vector<cv::Point2f> output_points;
    // TODO: looks like 4k video is having trouble detecing points..
    // this also might just be a upscaled video problem. Will ignore for now
    // something interesting may be to only smooth closer to the boundry,
    // so that cornering is less likely to pick things on the boundry
    cv::goodFeaturesToTrack(frame_gray, output_points, 70, 0.1, 
                            7, cv::noArray(), 7);

    PointsHistory ret = {
        .points = std::vector<SinglePointHistory>(output_points.size()),
        .valid = true,
        .point_size = 1 + static_cast<int>(std::sqrt(dims.area()) / 1000),
    };

    for (int i = 0; i < output_points.size(); i++) {
        // storage of the points has origin at rotation center, but good
        // features to track returns at with video origin
        ret.points[i].history.push_back(output_points[i] - center);
        ret.points[i].color = CV_RGB(randColor(), randColor(), randColor());
        ret.points[i].valid = true;
    }
    return ret;
}

PointsHistory manual_getpoints(std::string filename, std::string input,
                               cv::Point2f center) {
    auto randColor = [] { return 128+(rand() % static_cast<int>(128)); };
    auto vid = cv::VideoCapture(filename);

    auto dims = cv::Size(vid.get(cv::CAP_PROP_FRAME_WIDTH),
                         vid.get(cv::CAP_PROP_FRAME_HEIGHT));

    auto separated_input = deserialize(input);
    if (separated_input.size() % 2 != 0) {
        return PointsHistory{.valid = false};
    }

    PointsHistory ret = {
        .points = std::vector<SinglePointHistory>(separated_input.size() / 2),
        .valid = true,
        .point_size = 1 + static_cast<int>(std::sqrt(dims.area()) / 1000),
    };

    for (int i = 0; i < separated_input.size(); i += 2) {
        // js gives points with video origin, convert to rotation origin
        ret.points[i/2].history.push_back(
                cv::Point2f(strtod(separated_input[i].c_str(), NULL),
                            strtod(separated_input[i+1].c_str(), NULL))
                - center);
        ret.points[i].color = CV_RGB(randColor(), randColor(), randColor());
        ret.points[i].valid = true;
    }
    return ret;
}

PointsHistory adv_getpoints(std::string filename, cv::Point2f center,
                            bool is_auto, std::string adv_data) {
    srand(42); // always pick the same order of colors
    return is_auto ? 
        auto_getpoints(filename, adv_data, center) :
        manual_getpoints(filename, adv_data, center);
}

// gradient and polynomial methods

std::vector<double> gradient(std::vector<double> y, std::vector<double> x) {
    // numpy.gradient uses first order difference at boundaries and second
    // order central difference. assume x stepsize is constant (evenly spaced)
    std::vector<double> out(y.size());
    if (x.size() != y.size())
        return out;
    double h = x[1] - x[0];
    out[0] = (y[1] - y[0]) / h;
    int i = 1;
    while (i < out.size() - 1) {
        out[i] = 0.5 * (y[i+1] - y[i-1]) / h;
        i++;
    }
    out[i] = (y[i] - y[i-1]) / h;
    return out;
}

std::vector<double> polyfit(std::vector<double> x, std::vector<double> y, int k) {
    // x, y are n-by-1 vectors
    // construct the X matrix, so that
    // 1 x1 x1^2 .. x1^k
    // ..
    // 1 xn xn^2 .. xn^k is a n by (k+1) matrix,
    // a is a (k+1) by 1 vector, and y is a n by 1 vector,
    // and Xa = y is the equation to solve. feed into cv::solve with the normal flag,
    // so X^T X a = X^T y is what is actually solved. use SVD with is O(n^3)
    int n = x.size();
    std::vector<double> a_vec;
    if (y.size() != n || k > n)
        return a_vec;

    cv::Mat_<double> X(n, k+1, 1.0);
    for (size_t r = 0; r < n; r++) {
        double x_r = x[r];
        double x_cur = x_r;
        double* Xi = X.ptr<double>(r);
        for (size_t c = 1; c < X.cols; c++) {
            Xi[c] = x_cur;
            x_cur *= x_r;
        }
    }
    cv::Mat_<double> y_mat(y);
    cv::Mat_<double> a;
    cv::solve(X, y_mat, a, cv::DECOMP_SVD && cv::DECOMP_NORMAL);

    a_vec.assign(a.begin(), a.end());
    return a_vec;
}

double polyval(std::vector<double> a, double x) {
    double out = 0;
    double x_cur = 1;
    for (auto a_i : a) {
        out += a_i * x_cur;
        x_cur *= x;
    }
    return out;
}

#endif
