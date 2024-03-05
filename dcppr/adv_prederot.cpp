// general idea for prederot: its possible skipping six frames (or however many)
// is too hard for pyrLK tracking. In that case, you'll have to track 10 seconds
// of frames..
// preview of derotation
#include "adv_argutils.hpp"
#include <string>
#include <chrono>
#include <errno.h>
#include <libgen.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>

auto codec = cv::VideoWriter::fourcc('a', 'v', 'c', '1');

int main(int argc, const char* argv[]) {
    // ./bin FILE X Y R RPM advIsAuto advData trackForce outfn
    // advIsAuto == true -> advData: r_a (single number, inner circle radius)
    // advIsAuto == false -> advData: x,y... (up to 10 pairs of points)
    //if (argc != 9)
    //    return(-1);
    std::string filename = std::string(argv[1]);
    auto circ = cv::Point2f(strtod(argv[2], NULL), strtod(argv[3], NULL));
    int radii = atoi(argv[4]);
    double rpm = strtod(argv[5], NULL);
    bool advIsAuto = std::string(argv[6]) == "1";
    std::string adv_data = std::string(argv[7]);
    bool exportCsv = std::string(argv[10]) == "1";
    std::string outfn = std::string(argv[9]);

    if (!rpm || errno == ERANGE)
        return(-1);
    auto vid = cv::VideoCapture(filename);
    cv::Mat vid_frame;
    if (!vid.read(vid_frame))
        return(-2);
    PointsHistory pth = adv_getpoints(filename, advData, circ, advIsAuto);
    if (!pth.valid)
        return(-3);
    pth.t.push_back(0); // frame 0 starts at time 

    const int video_fps = vid_cap.get(cv::CAP_PROP_FPS);
    const auto video_dims = cv::Size(vid_cap.get(cv::CAP_PROP_FRAME_WIDTH),
                                     vid_cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    cv::Mat mask_frame = cv::Mat::zeros(video_dims, CV_8UC1);
    cv::circle(mask_frame, rotation_center, radii, 255, -1);
    cv::bitwise_not(mask_frame, mask_frame);

    const std::string overlay_txt_up = "VISUAL PREVIEW";
    const std::string overlay_txt_low = "CLICK \'DEROTATE\' FOR FULL QUALITY VIDEO";
    const auto overlay_orig_up = cv::Point(0, 30);
    const auto overlay_orig_low = cv::Point(0, 65);
    const auto txt_color = cv::RGB(255, 0, 0);

    const int kFramesToSkip = video_fps / 10 > 0 ? video_fps / 10 : 1;
    const int kOutputFramesLimit = std::min(100*kFramesToSkip, (int)(vid_cap.get(cv::CAP_PROP_FRAME_COUNT)*0.5));
    const double kdTheta = -6 * rpm / (double) video_fps;

    auto vidout = cv::VideoWriter(outfn, codec, fps < 10 ? fps : 10, dims);
    if (!vidout.isOpened())
        return(-2);

    std::vector<unsigned char> adv_status;
    std::vector<float> adv_error;
    cv::Mat adv_frame_gray;
    cv::Mat adv_frame_gray_old;
    std::vector<cv::Point2f> adv_points;
    std::vector<cv::Point2f> adv_points_old;
    for (int i = 0; i < pth.points.size(); i++) {
        adv_points_old.push_back(pth.points[i].history.back() + circ);
    }
    cur_rotation += dtheta;
    cv::cvtColor(vid_frame, adv_frame_gray_old, cv::COLOR_BGR2GRAY);
    vid.read(vid_frame);

    int n_frame = 1; 
    while(n_frame < kOutputFramesLimit) {
        cv::warpAffine(vid_frame, vid_frame, 
                       cv::getRotationMatrix2D(rotation_center, n_frame * kdTheta, 1.0),
                       video_dims);
        cv::bitwise_and(vid_frame, 0, vid_frame, mask_frame);
        cv::putText(vid_frame, overlay_txt_up, overlay_orig_up, cv_font, 1, txt_color, 2, cv::LINE_AA);
        cv::putText(vid_frame, overlay_txt_low, overlay_orig_low, cv_font, 1, txt_color, 2, cv::LINE_AA);
        vid_out << vid_frame;
        for (int j = 0; j < kFramesToSkip; j++)
            vid_cap.read(vid_frame);
        n_frame += kFramesToSkip;
    }

    vidout.release();
    return EXIT_SUCCESS;
}
