#ifndef _ADV_ARGUTILS_H_
#define _ADV_ARGUTILS_H_

#include "layoututils.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/core/base.hpp>
#include <opencv2/core/core.hpp>
#include <vector>
#include <unordered_map>

typedef struct {
    cv::Point2f dp;
    double r;
    double theta;
    double velocity_norm;
    double dr;
    double dtheta;
    cv::Point2f radiusVis;
} ExtraHistoryData;

typedef struct {
    // TODO: this probably needs rule of five? what would
    // swap do for something like this
    std::vector<cv::Point2f> history;
    // extra is missing one data point at beginning and one at end;
    // each i-1 in extra maps to i in history. do this because we can't
    // do central difference on the first and last points.
    std::vector<ExtraHistoryData> extra;
    bool valid;
    cv::Scalar color;
} SinglePointHistory;

typedef struct {
    std::vector<SinglePointHistory> points;
    // TODO: no need to log t -- unused anywhere, and can be generated
    // from frame outputs
    std::vector<float> t;
    bool valid;
    int point_size;
} PointsHistory;

/*
 * move-only vector wrapper with no bounds checking. Inserts never fail and
 * overwrite in a FIFO manner. Elements cannot be removed other by overwrite.
 * TODO: use this class instead of std::queue for the output buffer
 */
template<typename T>
class CircularBuffer {
    CircularBuffer<T>(size_t s) : max_size(s), cur_start_idx(0) {};

    size_t size() {
        return store.size();
    }

    void insert(T&& e) {
        if (store.size() == max_size) {
            store[cur_start_idx] = std::move(e);
            cur_start_idx = (++cur_start_idx) % store.size();
        } else
            store.push_back(std::move(e));
    }

    T& front() {
        return store.at(cur_start_idx);
    }

    class iterator : public std::iterator<std::input_iterator_tag, T> {
        size_t i = cur_start_idx;
        explicit iterator(size_t _i = 0, size_t _end = 0) : i(_i) {};
        iterator& operator++() {
            if (++i > store.size())
                i = 0;
            return *this;
        }
        iterator operator++(int) {
            iterator ret = *this; ++(*this); return ret;
        }
        bool operator==(iterator other) { return i == other.i; }
        bool operator!=(iterator other) { return !(this == other); }
        T& operator*() const { return store[i]; }
    };

    iterator begin() { return iterator(cur_start_idx); }
    iterator end() { return iterator(cur_start_idx == 0 ? store.size() : cur_start_idx - 1); }
  private:
    size_t cur_start_idx;
    size_t max_size;
    std::vector<T> store;
};

//NOLINTNEXTLINE(misc-definitions-in-headers)
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

//NOLINTNEXTLINE(misc-definitions-in-headers)
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

//NOLINTNEXTLINE(misc-definitions-in-headers)
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

//NOLINTNEXTLINE(misc-definitions-in-headers)
PointsHistory adv_getpoints(std::string filename, cv::Point2f center,
                            bool is_auto, std::string adv_data) {
    srand(42); // always pick the same order of colors
    return is_auto ?
        auto_getpoints(filename, adv_data, center) :
        manual_getpoints(filename, adv_data, center);
}

// gradient and polynomial methods

//NOLINTNEXTLINE(misc-definitions-in-headers)
std::vector<double> gradient(std::vector<double> y, std::vector<double> x) {
    // TODO: eventually refactor this to use iterators of vectors, rather than copy vectors
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

// this is gradient, but maybe you should make it more obvious its an online gradient
//NOLINTNEXTLINE(misc-definitions-in-headers)
cv::Point2f gradient3p(std::vector<cv::Point2f> points, std::vector<double> t) {
    std::vector<double> x;
    std::vector<double> y;
    for (auto i : points) {
        x.push_back(i.x);
        y.push_back(i.y);
    }
    x = gradient(x, t);
    y = gradient(y, t);
    return cv::Point2f(x[1], y[1]);
}

//NOLINTNEXTLINE(misc-definitions-in-headers)
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

//NOLINTNEXTLINE(misc-definitions-in-headers)
double polyval(std::vector<double> a, double x) {
    double out = 0;
    double x_cur = 1;
    for (auto a_i : a) {
        out += a_i * x_cur;
        x_cur *= x;
    }
    return out;
}

//NOLINTNEXTLINE(misc-definitions-in-headers)
void fill_extra_data(PointsHistory& p) {
    std::vector<double> t(p.t.rbegin(), p.t.rbegin() + 3);
    for (auto& kph : p.points) {
        ExtraHistoryData kpe;
        if (kph.valid && kph.history.size() > 2) {
            cv::Point2f& kph_2ndlast = *std::next(kph.history.rbegin() + 1);
            // TODO: r seems to not be constant?
            kpe.r = cv::norm(kph_2ndlast);
            kpe.theta = std::atan2(kph_2ndlast.y, kph_2ndlast.x);
            std::vector<cv::Point2f> points(kph.history.rbegin(),
                                            kph.history.rbegin() + 3);
            kpe.dp = gradient3p(points, t);
            kpe.velocity_norm = cv::norm(kpe.dp);
            std::vector<double> r;
            std::vector<double> theta;
            for (auto p : points) {
                r.push_back(cv::norm(p));
                theta.push_back(std::atan2(p.y, p.x));
            }
            // also gradient3p, but with vectors instead
            kpe.dr = gradient(r, t)[1];
            kpe.dtheta = gradient(theta, t)[1];
        }
        kph.extra.push_back(kpe);
    }
}

//NOLINTNEXTLINE(misc-definitions-in-headers)
void fill_draw_vis_res(PointsHistory& p, cv::Mat& frame, const int offset,
                       const double kRPM, const cv::Point2f& kRotCenterOffset) {
    const double totOmega = kRPM * 4 * M_PI / 60.0;
    // assume data is at rbegin() + offset, and that 2*offset + 1 data are avail
    for (SinglePointHistory& kph : p.points) {
        if (!kph.valid) continue;
        auto icp = std::next(kph.history.rbegin(), offset);
        auto icpe = std::next(kph.extra.rbegin(), offset-1);
        cv::arrowedLine(frame, *icp + kRotCenterOffset, *icp + icpe->dp + kRotCenterOffset, COLOR_BLUE);

        std::vector<double> fit_v;
        std::vector<double> fit_dx;
        std::vector<double> fit_dy;
        // TODO: for smoothing, t doesn't matter; make constexpr
        std::vector<double> t;
        double x = 3;
        for (auto i = std::next(icpe, offset); i >= kph.extra.rbegin(); --i, ++x) {
            fit_v.push_back(i->velocity_norm / totOmega);
            fit_dx.push_back(i->dp.x);
            fit_dy.push_back(i->dp.y);
            t.push_back(x);
        }
        double mid = t[t.size() / 2];
        double velnorm_smooth = polyval(polyfit(t, fit_v, 3), mid);
        double x_smooth = polyval(polyfit(t, fit_dx, 3), mid);
        double y_smooth = polyval(polyfit(t, fit_dy, 3), mid);
        double angle = atan2(y_smooth, x_smooth);
        icpe->radiusVis = *icp + cv::Point2f(-velnorm_smooth * std::sin(angle),
                                             velnorm_smooth * std::cos(angle));
        cv::line(frame, *icp + kRotCenterOffset, icpe->radiusVis + kRotCenterOffset, COLOR_WHITE);
    }
}

//NOLINTNEXTLINE(misc-definitions-in-headers)
void export_csv(std::string fname, PointsHistory& p) {
    std::ofstream f(fname);
    for (int i = 0; i < p.points.size(); i++) {
        auto is = std::to_string(i);
        f << is + "x," << is + "y," << is + "dx," << is + "dy,"
          << is + "||velocity||," << is + "r," << is + "dr," << is + "theta,"
          << is + "dtheta," << is + "radiusx," << is + "radiusy" << std::endl;
    }

    // non-locality iter to allow entire line to be fully written
    for (int j = 0; j < p.t.size(); j++) {
        for (int i = 0; i < p.points.size(); i++) {
            if (p.points[i].history.size() < j) {
                f << ",,,,,,,,,,,";
            } else {
                f << p.points[i].history[j].x << "," << p.points[i].history[j].y << ",";
                if (j == 0 || p.points[i].extra.size() < j) {
                    f << ",,,,,,,,,";
                } else {
                    // TODO: extra radius point is unitializaed for lots?
                    auto& e = p.points[i].extra[j];
                    f << e.dp.x << ',' << e.dp.y << ',' << e.velocity_norm << ','
                      << e.r << ',' << e.dr << ',' << e.theta << ',' << e.dtheta
                      << ',' << e.radiusVis.x << ',' << e.radiusVis.y << ',';
                }
            }
            f << std::endl;
        }
    }
}

#endif
