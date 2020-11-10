#include <string>
#include <chrono>
#include <errno.h>
#include <libgen.h>
#include <opencv2/opencv.hpp>

const std::string allowed = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
auto codec = cv::VideoWriter::fourcc('a', 'v', 'c', '1');

std::string randstr() {
    std::string out (10, 'x');
    for (int i = 0; i < 10; i++)
        out[i] = allowed[std::rand() % allowed.length()];
    return out;
}

int main(int argc, const char* argv[]) {
    //std::srand((unsigned int) std::chrono::system_clock::now().time_since_epoch().count());

    // ./bin FILE X Y R RPM OUTFILE
    if (argc != 7)
        return(-1);
    char* filename = (char*) argv[1];
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
    int total_frames = vid.get(cv::CAP_PROP_FRAME_COUNT);
    auto dims = cv::Size(vid.get(cv::CAP_PROP_FRAME_WIDTH), vid.get(cv::CAP_PROP_FRAME_HEIGHT));
    cv::Mat center_mask = cv::Mat::zeros(dims, CV_8UC1);
    cv::Mat out_frame;
    cv::circle(center_mask, circ, radii, 255, -1);

    auto vidout = cv::VideoWriter(outfn, codec, fps, dims);
    int i = 0;
    double dtheta = -6 * rpm / (double) fps;

    if (!vidout.isOpened())
        return(-2);

    do {
        cv::warpAffine(vid_frame, vid_frame, cv::getRotationMatrix2D(circ, i++ * dtheta, 1.0), dims);
        vid_frame.copyTo(out_frame, center_mask);
        vidout << out_frame;
    } while(vid.read(vid_frame));

    vidout.release();
    return EXIT_SUCCESS;
}
