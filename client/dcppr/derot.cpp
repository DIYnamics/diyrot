#include <iostream>
#include <string>
#include <chrono>
#include <errno.h>
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
    std::srand((unsigned int) std::chrono::system_clock::now().time_since_epoch().count());

    // ./bin FILE R X Y RPM
    if (argc != 6)
        return(-1);
    const char* filename = argv[1];
    auto circ = cv::Point(atoi(argv[2]), atoi(argv[3]));
    int radii = atoi(argv[4]);
    double rpm = strtod(argv[5], NULL);
    if (!rpm || errno == ERANGE)
        return(-1);
    auto vid = cv::VideoCapture(filename);
    cv::Mat frame;
    if (!vid.read(frame))
        return(-2);

    int fps = vid.get(cv::CAP_PROP_FPS);
    int total_frames = vid.get(cv::CAP_PROP_FRAME_COUNT);
    auto dims = cv::Size(vid.get(cv::CAP_PROP_FRAME_WIDTH), vid.get(cv::CAP_PROP_FRAME_HEIGHT));
    double dtheta = -6 * rpm / (double) fps;
    auto vidoutfn = randstr()+std::string(filename);
    auto vidout = cv::VideoWriter("../uploads"+vidoutfn, codec, fps, dims);

    int i = 0;
    while(vid.read(frame)) {
        cv::warpAffine(frame, frame, cv::getRotationMatrix2D(circ, (double)i++ * dtheta, 1.0), dims);
        vidout << frame;
    }
    vidout.release();
    std::cout << vidoutfn << std::endl;
    return EXIT_SUCCESS;
}
