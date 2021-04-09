#include <string>
#include <chrono>
#include <errno.h>
#include <libgen.h>
#include <opencv2/opencv.hpp>

auto codec = cv::VideoWriter::fourcc('a', 'v', 'c', '1');

// generates a 'preview' of the derotated video.
// this file is largely a copy of derot.cpp, so only interesting lines have
// been commented.
int main(int argc, const char* argv[]) {
   // ./bin FILE X Y R RPM OUTFILE
    if (argc != 7)
        return(-1);
    std::string filename = std::string(argv[1]);
    auto circ = cv::Point2f(strtod(argv[2], NULL), strtod(argv[3], NULL));
    int radii = atoi(argv[4]);
    double rpm = strtod(argv[5], NULL);
    std::string outfn = std::string(argv[6]);
    if (!rpm || errno == ERANGE)
        return(-1);
    auto vid = cv::VideoCapture(filename);
    cv::Mat vid_frame;
    if (!vid.read(vid_frame))
        return(-2);

    int fps = vid.get(cv::CAP_PROP_FPS);
    auto dims = cv::Size(vid.get(cv::CAP_PROP_FRAME_WIDTH), vid.get(cv::CAP_PROP_FRAME_HEIGHT));
    cv::Mat center_mask = cv::Mat::zeros(dims, CV_8UC1);
    cv::circle(center_mask, circ, radii, 255, -1);
    cv::bitwise_not(center_mask, center_mask);

    // preview emphasis is on speed, so interested in getting video out fast.
    // specify how many frames get read but not processed - 
    // 'skipped' for the output video
    int skip = fps / 10;
    // reduce framerate to 10fps 
    auto vidout = cv::VideoWriter(outfn, codec, 10, dims);
    int i = 0;
    int j = 0;
    double dtheta = -6 * rpm / (double) fps;

    if (!vidout.isOpened())
        return(-2);

    std::string overlaytxtup = "VISUAL PREVIEW";
    std::string overlaytxtlow = "CLICK \'DEROTATE\' FOR FULL QUALITY VIDEO";
    auto origup = cv::Point(0, (int)vid.get(cv::CAP_PROP_FRAME_HEIGHT)-30);
    auto origlow = cv::Point(0, (int)vid.get(cv::CAP_PROP_FRAME_HEIGHT)-5);
    auto color = cv::Scalar(0, 0, 255);

    // main loop is a bit different - around 100 frames are written to output
    // vid, which is ~10 seconds. to do this, we loop to quickly read and discard
    // the skipped frames (j for loop). when hit non-skip frame (exit from loop),
    // set dtheta to the expected amount based on number of frames skipped, and write.

    // read 10 sec or half the amount of existing frames
    int lim = std::min(100*skip, (int)(vid.get(cv::CAP_PROP_FRAME_COUNT)*0.5));
    
    while(i < lim) {
        for (j = 0; j < skip; j++)
            vid.read(vid_frame);
        i += skip;
        cv::warpAffine(vid_frame, vid_frame, cv::getRotationMatrix2D(circ, i * dtheta, 1.0), dims);
        cv::bitwise_and(vid_frame, 0, vid_frame, center_mask);
        cv::putText(vid_frame, overlaytxtup, origup, cv::FONT_HERSHEY_COMPLEX_SMALL, 1, color, 2, cv::LINE_AA);
        cv::putText(vid_frame, overlaytxtlow, origlow, cv::FONT_HERSHEY_COMPLEX_SMALL, 1, color, 2, cv::LINE_AA);
        vidout << vid_frame;
    }

    vidout.release();
    return EXIT_SUCCESS;
}
