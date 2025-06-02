// hough circle detection
#include <iostream>
#include <stdlib.h>
#include <errno.h>
#include <opencv2/opencv.hpp>

// tries to automatically detect a circle using hough transformations
// ./radii_check FILE_FULL_PATH PREVIEW_FULL_PATH
int main(int argc, const char* argv[]) {
    // if no file argument, fail
    if (argc < 3)
        return(-1);
    // extract filename and preview img fname
    const auto filename = std::string(argv[1]);
    const auto preview_img_fn = std::string(argv[2]);
    // open video
    auto in_vidcap = cv::VideoCapture(filename);

    cv::Mat working_frame;
    // extract video frame, or fail
    if (!in_vidcap.read(working_frame))
        return(-2);

    // write a jpeg of the first frame for frontend use
    cv::imwrite(preview_img_fn, working_frame);

    const cv::Size kWorkingFrameDims = working_frame.size();

    // convert read video frame to grayscale
    cv::Mat working_frame_gray;
    cv::cvtColor(working_frame, working_frame_gray, cv::COLOR_BGR2GRAY);
    // gaussian blur said frame (make circle detection more robust)
    cv::Mat working_frame_blur;
    cv::medianBlur(working_frame_gray, working_frame_blur, 5);
    // vector of cv::Vec (collection of circles)
    std::vector<cv::Vec3f> circles;

    // ask opencv to find valid circles. see documentation; most parameters are
    // impossible to tune main one here is minDist = std::min(...), which says
    // the distance between circles centers should be 30% of the smaller of the
    // two video dimensions. While 30% is arbitrary, it does prevent hough from
    // finding too many circles.
    cv::HoughCircles(working_frame_blur,
                    circles,
                    cv::HOUGH_GRADIENT,
                    1.5, 
                    0.3 * std::min(kWorkingFrameDims.height, 
                                   kWorkingFrameDims.width),
                    300,
                    100);

    // if no circles found, return.
    // This is a benign error (we can manually specify), but needs to not conflict
    // with the error codes.
    if (circles.size() < 1)
        return(100);

    int x = circles[0][0];
    int y = circles[0][1];
    int r = circles[0][2];

    // print first circle: x, y, r
    std::cout << x << " " << y << " " << r;

    // close video, exit
    in_vidcap.release();
    return EXIT_SUCCESS;
}
