#define CODEC (cv::VideoWriter::fourcc('a', 'v', 'c', '1'))

#define MAX_VIDEO_FRAME_AREA (5120*2880)
#define MIN_VIDEO_FRAME_AREA (400*300)
// # of trailing points that get drawn, as well as half the size of the radius
// visualization window
#define K_TRAILING_POINTS (15)

#if defined(PRE) || defined(ADV_PRE)
#define PREVIEW 1
#endif
#if defined(ADV_DEROT) || defined(ADV_PRE)
#define ADVANCED 1
#endif

#if !defined(ADVANCED)
#define ARGC_COUNT 7
#elif defined(ADV_PRE)
#define ARGC_COUNT 11
#elif defined(ADV_DEROT)
#define ARGC_COUNT 12
#endif

#include "adv_utils.hpp"
#include "layoututils.hpp"
#include <cstdio>
#include <cmath>
#include <errno.h>
#include <list>
#include <opencv2/core/types.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/video/tracking.hpp>
#include <queue>
#include <string>
#include <sstream>
#include <vector>

/*
    Argument API:
    0 ./bin
    1     FILENAME:str
    2     X:double
    3     Y:double
    4     R:double
    5     RPM:double
    6     FILENAME:str
    7  [  ADV_AUTO:'0'/'1'
    8     ADV_DATA:str
    9     ADV_VIS:'0'/'1'
    10    ADV_LONG:'0'/'1'
    11  [ ADV_CSV:'0'/'1' ]
       ]
*/

int main(int argc, const char* argv[]) {
    if (argc != ARGC_COUNT)
        return -1;
    const std::string kInputFn = std::string(argv[1]);
    const auto kRotCenter =
        cv::Point2f(strtod(argv[2], NULL), strtod(argv[3], NULL));
    const double kRotRadius = strtod(argv[4], NULL);
    const double kRPM = strtod(argv[5], NULL);
    const std::string kOutputFn = std::string(argv[6]);
#if defined(ADVANCED)
    const bool kAdvAuto = std::string(argv[7]) == "1";
    const std::string kAdvData = std::string(argv[8]);
    // don't split kVisRadius out as macro enabled as its hard
    const bool kVisRadius = std::string(argv[9]) == "1";
    const bool kAdvFullTrail = std::string(argv[10]) == "1";
    const int kOutputQueueSize = kVisRadius ? K_TRAILING_POINTS : 1;
#if defined(ADV_DEROT)
    // TODO: don't allocate the extra data vector if not exporting or visualize
    const bool kExportCsv = std::string(argv[11]) == "1";
#endif
#endif

    if (!kRPM || errno == ERANGE)
        return -1; // something happened with input processing

    auto in_vidcap = cv::VideoCapture(kInputFn);
    const double kInVidFPS = in_vidcap.get(cv::CAP_PROP_FPS);
    const int kInVidFrameCount = in_vidcap.get(cv::CAP_PROP_FRAME_COUNT);

    cv::Mat working_frame;
    if (!in_vidcap.read(working_frame))
        return -2;
    const cv::Size kWorkingFrameSize = working_frame.size();

    // don't try to derotate anormally sized videos
    if (kWorkingFrameSize.area() >= MAX_VIDEO_FRAME_AREA ||
        kWorkingFrameSize.area() <= MIN_VIDEO_FRAME_AREA)
        return -3;

    // already read the first frame
    int n_frame = 1;
    const double kdTheta = -6.0 * kRPM / kInVidFPS;

    const cv::Rect kWorkingFrameROI = cv::Rect(
        cv::Point(std::max(static_cast<double>(0), kRotCenter.x - kRotRadius),
                  std::max(static_cast<double>(0), kRotCenter.y - kRotRadius)),
        cv::Point(std::min(static_cast<double>(kWorkingFrameSize.width),
                           kRotCenter.x + kRotRadius),
                  std::min(static_cast<double>(kWorkingFrameSize.height),
                           kRotCenter.y + kRotRadius)));

    auto layout = makeLayout(kWorkingFrameROI.size(), kRPM);

#if defined(SIDE_BY_SIDE)
    const auto kOutOrigFrameROI =
        cv::Rect(layout.ogFrameOrigin, kWorkingFrameROI.size());
#endif
    const auto kOutDerotFrameROI =
        cv::Rect(layout.derotFrameOrigin, kWorkingFrameROI.size());

    cv::Mat output_frame = cv::Mat::zeros(layout.outputFrameSize,
                                          working_frame.type());

    // name of the file VideoWriter writes to initially
    std::string write_fn = kOutputFn;
    double out_fps;
#if defined(PREVIEW)
    // skip in-between frames to roughly reduce output frames per second to 10
    const int kFramesToSkip = std::max(kInVidFPS / 10.0, 1.0);
    // read 10 sec * 10 fps or half the amount of existing frames
    const int kOutputFramesLimit = std::min(100 * kFramesToSkip,
                                            kInVidFrameCount);
    out_fps = 10.0;
#else
    // full derotations require a TMP file else server immediately reports done
    auto basename_pos = kOutputFn.find_last_of('/');
    basename_pos = (basename_pos == std::string::npos) ? 0 : basename_pos + 1;
    write_fn.insert(basename_pos, "TMP");
    out_fps = kInVidFPS;
#endif

    auto output_vidwriter = cv::VideoWriter(write_fn, CODEC,
                                            out_fps, layout.outputFrameSize);
    if (!output_vidwriter.isOpened())
        return -4;

    // mask of the working frame, so that ROI is 1 and everything else is 0.
    cv::Mat rot_mask = cv::Mat::zeros(kWorkingFrameSize, CV_8UC1);
    cv::circle(rot_mask, kRotCenter, kRotRadius, 255, -1);
    cv::bitwise_not(rot_mask, rot_mask);

    // draw text a single time in areas that won't be overwritten
    const auto kInfoTextMat =
        drawString(makeInfoString(kRPM), layout.quotient, output_frame.type());
    const auto kAttrTextMat =
        drawString(kAttrString, layout.quotient, output_frame.type());
    const auto kInfoTextROI = cv::Rect(layout.infoTextOrigin, kInfoTextMat.size());
    const auto kAttrTextROI = cv::Rect(layout.attrTextOrigin, kAttrTextMat.size());
    kInfoTextMat.copyTo(output_frame(kInfoTextROI));
    kAttrTextMat.copyTo(output_frame(kAttrTextROI));
#if defined(SIDE_BY_SIDE)
    const auto kOgLabelTextMat =
        drawString(kSbsOrigLabel, layout.quotient, output_frame.type());
    const auto kDerotLabelTextMat =
        drawString(kSbsDerotLabel, layout.quotient, output_frame.type());
    const auto kOgLabelTextROI = cv::Rect(layout.ogLabelOrigin, kOgLabelTextMat.size());
    const auto kDerotLabelTextROI = cv::Rect(layout.derotLabelOrigin, kDerotLabelTextMat.size());
#endif

#if defined(ADVANCED)
    std::queue<cv::Mat, std::list<cv::Mat>> output_frame_queue;
    cv::Point2f kRotCenterInOutputFrame =
        static_cast<cv::Point2f>(layout.derotFrameOrigin) + kRotCenter -
        static_cast<cv::Point2f>(kWorkingFrameROI.tl());

    PointsHistory adv_pth =
        adv_getpoints(kInputFn, kRotCenter, kAdvAuto, kAdvData);
    if (!adv_pth.valid)
        return -5;
    adv_pth.t.push_back(0); // frame 0 starts at time 0
    std::vector<unsigned char> adv_lkpyr_status;
    std::vector<float> adv_lkpyr_error;
    cv::Mat adv_frame_gray;
    cv::Mat adv_frame_gray_old;

    // vector keeps track in video origin
    std::vector<cv::Point2f> adv_lkpyr_points;
    std::vector<cv::Point2f> adv_lkpyr_points_old;
    for (int i = 0; i < adv_pth.points.size(); i++) {
        adv_lkpyr_points_old.push_back(adv_pth.points[i].history.back() +
                                       kRotCenter);
    }

    cv::cvtColor(working_frame, adv_frame_gray_old, cv::COLOR_BGR2GRAY);
    // read a frame - a fresh one is needed to start LKPyr; don't show this one
    in_vidcap.read(working_frame);
    n_frame++;
#endif

    do {
#if defined(SIDE_BY_SIDE)
        // copy original frame to output
        working_frame(kWorkingFrameROI).copyTo(output_frame(kOutOrigFrameROI));
#endif
        cv::bitwise_and(working_frame, 0, working_frame, rot_mask);
        cv::warpAffine(working_frame, working_frame,
                       cv::getRotationMatrix2D(kRotCenter,
                                               n_frame * kdTheta,
                                               1.0),
                       kWorkingFrameSize);
#if defined(ADVANCED) // all adv need frame-by-frame tracking
        adv_pth.t.push_back(adv_pth.t.size() / kInVidFPS);
        if (adv_pth.valid) {
            cv::cvtColor(working_frame, adv_frame_gray, cv::COLOR_BGR2GRAY);
            cv::calcOpticalFlowPyrLK(adv_frame_gray_old,
                                     adv_frame_gray,
                                     adv_lkpyr_points_old,
                                     adv_lkpyr_points, // three args of output
                                     adv_lkpyr_status,
                                     adv_lkpyr_error,
                                     cv::Size(10, 10), // window size
                                     2,
                                     cv::TermCriteria(cv::TermCriteria::COUNT +
                                                      cv::TermCriteria::EPS,
                                                      20,
                                                      0.05));
            adv_frame_gray_old = adv_frame_gray.clone();

            // the assumed invariants here are that pth.points has all valid
            // points before non valid points and that adv_points_old has all
            // the valid points in the same order as pth.points. during each
            // iteration, adv_points, old, and status will always have the same
            // count of points
            adv_pth.valid = false;
            int adv_points_demoted = 0;
            adv_lkpyr_points_old.clear();
            for (int i = 0; i < adv_lkpyr_points.size(); i++) {
                int cur_index = i - adv_points_demoted;
                SinglePointHistory &cur_pth_point = adv_pth.points[cur_index];
                auto cur_point_in_rotframe = adv_lkpyr_points[i] - kRotCenter;
                if (!adv_lkpyr_status[i] ||
                        cur_point_in_rotframe.dot(cur_point_in_rotframe) >=
                        kRotRadius * kRotRadius) {
                    cur_pth_point.valid = false;
                    std::swap(adv_pth.points[cur_index],
                              adv_pth.points[adv_pth.points.size() - 1]);
                    adv_points_demoted++;
                    continue;
                }
                cur_pth_point.history.push_back(cur_point_in_rotframe);

                adv_lkpyr_points_old.push_back(adv_lkpyr_points[i]);
                adv_pth.valid = true;
            }

            int starting_idx = kAdvFullTrail ? 0 : std::max(0, (int)adv_pth.t.size() - K_TRAILING_POINTS);
            // TODO: dynamically adjust alpha / # of points drawn if to avoid noisy images
            for (int i = 0; i < adv_lkpyr_points.size()-adv_points_demoted; i++) {
                for (int j = starting_idx; j <
                adv_pth.points[i].history.size(); j++) {
                    cv::circle(working_frame,
                               adv_pth.points[i].history[j]+kRotCenter,
                               adv_pth.point_size,
                               adv_pth.points[i].color,
                               -1);
                }
            }
            fill_extra_data(adv_pth);
        }
#endif

        working_frame(kWorkingFrameROI).copyTo(output_frame(kOutDerotFrameROI));
#if defined (SIDE_BY_SIDE)
        kOgLabelTextMat.copyTo(output_frame(kOgLabelTextROI));
        kDerotLabelTextMat.copyTo(output_frame(kDerotLabelTextROI));
#endif
#if defined(ADVANCED)
        if (n_frame < kOutputQueueSize) {
            #if defined(PREVIEW)
            if (n_frame % kFramesToSkip == 0) // only write some output frames
            #endif
            output_vidwriter << output_frame;
            continue;
        }
        output_frame_queue.push(std::move(output_frame.clone()));
        if (output_frame_queue.size() < kOutputQueueSize)
            continue;
        // common path: take the oldest item and fill in data that requires window
        output_frame = std::move(output_frame_queue.front());
        output_frame_queue.pop();
        if (kVisRadius)
            fill_draw_vis_res(adv_pth, output_frame, kOutputQueueSize-1, kRPM, kRotCenterInOutputFrame);
#endif
#if defined(PREVIEW)
        if (n_frame >= kOutputFramesLimit) // if we've met the frame limit, exit
            break;
        if (n_frame % kFramesToSkip != 0) // if we're in one of the skipped frames, continue
            continue;
#endif

        output_vidwriter << output_frame;
    } while (n_frame++, in_vidcap.read(working_frame));

#if defined(ADV_DEROT)
    while(!output_frame_queue.empty()) { // rampdown
        output_frame = std::move(output_frame_queue.front());
        output_frame_queue.pop();
        output_vidwriter << output_frame;
    }
#endif

    output_vidwriter.release();

#if defined(ADV_DEROT)
    if (kExportCsv)
        export_csv(kOutputFn + ".csv", adv_pth);
#endif
    rename(write_fn.c_str(), kOutputFn.c_str());

#if defined(ADVANCED)
    return adv_pth.valid ? EXIT_SUCCESS : 3; // no tracking points left
#else
    return EXIT_SUCCESS;
#endif
}
