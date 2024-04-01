#include <array>
#include <deque>
#include <iostream>
#include <unordered_map>
#include <opencv2/opencv.hpp>

#ifndef _ADV_ARGUTILS_H_
#define _ADV_ARGUTILS_H_
#define WINDOW 30

typedef struct {
    std::vector<cv::Point2f> history;
    std::vector<float> r;
    std::vector<float> theta;
    bool valid;
    cv::Scalar color;
} SinglePointHistory;

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

#endif
