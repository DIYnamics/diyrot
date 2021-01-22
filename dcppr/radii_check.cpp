// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html

#include <iostream>
#include <stdlib.h>
#include <errno.h>
#include <opencv2/opencv.hpp>

int main(int argc, const char* argv[]) {
    if (argc < 2)
        return(-1);
    const char* filename = argv[1];
    double radii;
    auto vid = cv::VideoCapture(filename);
    if (argc == 2)
        radii = std::min(vid.get(cv::CAP_PROP_FRAME_WIDTH), vid.get(cv::CAP_PROP_FRAME_HEIGHT)) * 0.3;
    else 
        radii = strtod(argv[2], NULL);
    if (!radii || errno == ERANGE)
        return(-1);
    cv::Mat frame;
    if (!vid.read(frame) || !radii)
        return(-2);

    cv::Mat frame_gray;
    cv::cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
    cv::Mat frame_blur;
    cv::medianBlur(frame_gray, frame_blur, 5);
    std::vector<cv::Vec3f> circles;

    cv::HoughCircles(frame_blur, circles, cv::HOUGH_GRADIENT, 1.5, 
        std::min(vid.get(cv::CAP_PROP_FRAME_WIDTH), vid.get(cv::CAP_PROP_FRAME_HEIGHT)) * 0.3, 300, 100);

    if (circles.size() < 1)
        return(-3);

    std::cout << (int)circles[0][0] << " " << (int)circles[0][1] << " " << (int)circles[0][2];

    vid.release();
    return EXIT_SUCCESS;
}
