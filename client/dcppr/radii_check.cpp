// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html

#include <iostream>
#include <stdlib.h>
#include <errno.h>
#include <opencv2/opencv.hpp>

int main(int argc, const char* argv[]) {
    if (argc != 3)
        return(-1);
    const char* filename = argv[1];
    double radii = strtod(argv[2], NULL);
    if (!radii || errno == ERANGE)
        return(-1);
    auto vid = cv::VideoCapture(filename);
    cv::Mat frame;
    if (!vid.read(frame) || !radii)
        return(-2);

    cv::Mat frame_gray;
    cv::cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
    cv::Mat frame_blur;
    cv::medianBlur(frame_gray, frame_blur, 5);
    std::vector<cv::Vec3f> circles;

    cv::HoughCircles(frame_blur, circles, cv::HOUGH_GRADIENT, 1.5, radii * 0.8, 300, 100, 250, 0);

    if (circles.size() < 1)
        return(-3);

    std::cout << (int)circles[0][0] << " " << (int)circles[0][1] << " " << (int)circles[0][2];

    vid.release();
    return EXIT_SUCCESS;
}
