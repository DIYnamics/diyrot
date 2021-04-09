#include <iostream>
#include <stdlib.h>
#include <errno.h>
#include <opencv2/opencv.hpp>

// tries to automatically detect a circle using hough transformations
// ./radii_check FILE 
int main(int argc, const char* argv[]) {
    // if no file argument, fail
    if (argc < 2)
        return(-1);
    // extract filename from first argumnent
    const char* filename = argv[1];
    // open video
    auto vid = cv::VideoCapture(filename);

    cv::Mat frame;
    // extract video frame, or fail
    if (!vid.read(frame))
        return(-2);

    // convert read video frame to grayscale
    cv::Mat frame_gray;
    cv::cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
    // gaussian blur said frame (make circle detection more robust)
    cv::Mat frame_blur;
    cv::medianBlur(frame_gray, frame_blur, 5);
    // vector of cv::Vec (collection of circles)
    std::vector<cv::Vec3f> circles;

    // ask opencv to find valid circles. see documentation; most parameters are
    // impossible to tune main one here is minDist = std::min(...), which says
    // the distance between circles centers should be 30% of the smaller of the
    // two video dimensions. While 30% is arbitrary, it does prevent hough from
    // finding too many circles.
    cv::HoughCircles(frame_blur, circles, cv::HOUGH_GRADIENT, 1.5, 
        std::min(vid.get(cv::CAP_PROP_FRAME_WIDTH), vid.get(cv::CAP_PROP_FRAME_HEIGHT)) * 0.3, 300, 100);

    // if no circles found, return
    if (circles.size() < 1)
        return(-3);

    // print first circle: x, y, r
    std::cout << (int)circles[0][0] << " " << (int)circles[0][1] << " " << (int)circles[0][2];

    // close video, exit
    vid.release();
    return EXIT_SUCCESS;
}
