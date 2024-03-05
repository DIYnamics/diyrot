// generate a preview of derotation
#include <string>
#include <chrono>
#include <errno.h>
#include <libgen.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>

using cv_font = cv::FONT_HERSHEY_COMPLEX_SMALL;
auto codec = cv::VideoWriter::fourcc('a', 'v', 'c', '1');

int main(int argc, const char* argv[]) {
   // ./bin FILE X Y R RPM OUTFILE
    if (argc != 7)
        return(-1);
    std::string filename = std::string(argv[1]);
    auto rotation_center = cv::Point2f(strtod(argv[2], NULL), strtod(argv[3], NULL));
    int radii = atoi(argv[4]);
    double rpm = strtod(argv[5], NULL);
    std::string output_fn = std::string(argv[6]);
    if (!rpm || errno == ERANGE)
        return(-1); // something happened with input processing

    auto vid_cap = cv::VideoCapture(filename);
    cv::Mat vid_frame;
    if (!vid_cap.read(vid_frame))
        return(-2);

    auto preview_fn = output_fn;
    preview_fn.append(".jpeg");
    cv::imwrite(preview_fn, vid_frame);

    // get reported frames per second and (width, height) dimentions of video
    const int video_fps = vid_cap.get(cv::CAP_PROP_FPS);
    const auto video_dims = cv::Size(vid_cap.get(cv::CAP_PROP_FRAME_WIDTH), vid_cap.get(cv::CAP_PROP_FRAME_HEIGHT));

    // create a mask so everything inside circle of rotation is 1, else 0
    cv::Mat mask_frame = cv::Mat::zeros(video_dims, CV_8UC1);
    cv::circle(mask_frame, rotation_center, radii, 255, -1);
    cv::bitwise_not(mask_frame, mask_frame);

    // specify drawing of text
    const std::string overlay_txt_up = "VISUAL PREVIEW";
    const std::string overlay_txt_low = "CLICK \'DEROTATE\' FOR FULL QUALITY VIDEO";
    const auto overlay_orig_up = cv::Point(0, 30);
    const auto overlay_orig_low = cv::Point(0, 65);
    const auto txt_color = cv::RGB(255, 0, 0);

    // skip in-between frames to reduce output frames per second to 10
    const int kFramesToSkip = video_fps / 10 > 0 ? video_fps / 10 : 1;
    // read 10 sec times 10 fps or half the amount of existing frames
    const int kOutputFramesLimit = std::min(100*kFramesToSkip, (int)(vid_cap.get(cv::CAP_PROP_FRAME_COUNT)*0.5));
    // change in RPM per frame
    const double kdTheta = -6 * rpm / (double) video_fps;

    auto vid_out = cv::VideoWriter(output_fn, codec, video_fps < 10 ? video_fps : 10, video_dims);
    if (!vid_out.isOpened())
        return(-2);

    // we have already read the first frame
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

    vid_out.release();
    return EXIT_SUCCESS;
}
