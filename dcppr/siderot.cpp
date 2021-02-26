#include <string>
#include <chrono>
#include <errno.h>
#include <libgen.h>
#include <opencv2/opencv.hpp>

auto codec = cv::VideoWriter::fourcc('a', 'v', 'c', '1');
int main(int argc, const char* argv[]) {
    // ./bin FILE X Y R RPM OUTFILE
    if (argc != 7)
        return(-1);
    char* filename = (char*) argv[1];
    int x = atoi(argv[2]);
    int y = atoi(argv[3]);
    auto circ = cv::Point(atoi(argv[2]), atoi(argv[3]));
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
    //int total_frames = vid.get(cv::CAP_PROP_FRAME_COUNT);
    auto vdims = cv::Size(vid.get(cv::CAP_PROP_FRAME_WIDTH), vid.get(cv::CAP_PROP_FRAME_HEIGHT));
    auto outdims = cv::Size(radii*4, radii*2);
    cv::Mat center_mask = cv::Mat::zeros(vdims, CV_8UC1);
    cv::Mat tmp;
    cv::Mat out_frame = cv::Mat::zeros(outdims, vid_frame.type());
    cv::circle(center_mask, circ, radii, 255, -1);

    auto vidout = cv::VideoWriter(outfn, codec, fps, outdims);
    int i = 0;
    double dtheta = -6 * rpm / (double) fps;

    if (!vidout.isOpened())
        return(-2);

    do {
        vid_frame(cv::Rect(x-radii, y-radii, 2*radii, 2*radii))
            .copyTo(out_frame(cv::Rect(radii*2, 0, radii*2, radii*2)));
        cv::warpAffine(vid_frame, vid_frame, cv::getRotationMatrix2D(circ, i++ * dtheta, 1.0), vdims);
        vid_frame.copyTo(tmp, center_mask);
        tmp(cv::Rect(x-radii, y-radii, 2*radii, 2*radii))
            .copyTo(out_frame(cv::Rect(0, 0, radii*2, radii*2)));
        vidout << out_frame;
    } while(vid.read(vid_frame));

    vidout.release();
    return EXIT_SUCCESS;
}
