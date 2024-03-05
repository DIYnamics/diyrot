// main derotation program, for object tracking.
// Comments are elided when similar to derot.cpp
#include "adv_argutils.hpp"
#include <array>
#include <string>
#include <sstream>
#include <cstdio>
#include <errno.h>
#include <opencv2/core/types.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/video/tracking.hpp>
#include <vector>

auto codec = cv::VideoWriter::fourcc('a', 'v', 'c', '1');

int main(int argc, const char* argv[]) {
    // ./bin FILE X Y R RPM sbs advIsAuto advData trackForce exportCsv outfn
    // advIsAuto == true -> advData: r_a (single number, inner circle radius)
    // advIsAuto == false -> advData: x,y... (up to 10 pairs of points)
    if (argc != 12)
        return(-1);
    std::string filename = std::string(argv[1]);
    auto circ = cv::Point2f(strtod(argv[2], NULL), strtod(argv[3], NULL));
    int radii = atoi(argv[4]);
    double rpm = strtod(argv[5], NULL);
    bool sideBySide = std::string(argv[6]) == "1";
    bool advIsAuto = std::string(argv[7]) == "1";
    std::string advData = std::string(argv[8]);
    bool trackForce = std::string(argv[9]) == "1";
    bool exportCsv = std::string(argv[10]) == "1";
    std::string outfn = std::string(argv[11]);
    std::string tmpfn = outfn;
    auto f = outfn.find_last_of("/");
    f = (f == std::string::npos) ? 0 : f+1;
    tmpfn.insert(f, "TMP");
    if (!rpm || errno == ERANGE)
        return(-1);
    auto vid = cv::VideoCapture(filename);
    cv::Mat vid_frame;
    if (!vid.read(vid_frame))
        return(-2);
    PointsHistory pth = adv_getpoints(filename, circ, advIsAuto, advData);
    if (!pth.valid)
        return(-3);
    pth.t.push_back(0); // frame 0 starts at time 0

    double fps = vid.get(cv::CAP_PROP_FPS);
    auto dims = cv::Size(vid.get(cv::CAP_PROP_FRAME_WIDTH), vid.get(cv::CAP_PROP_FRAME_HEIGHT));
    cv::Mat center_mask = cv::Mat::zeros(dims, CV_8UC1);
    cv::circle(center_mask, circ, radii, 255, -1);
    cv::bitwise_not(center_mask, center_mask);

    // TODO: need to check whether SBS mode interacts with advanced
    auto regin = cv::Rect(
        cv::Point(circ.x - radii > 0 ? circ.x - radii : 0,
            circ.y - radii > 0 ? circ.y - radii : 0),
        cv::Point(circ.x + radii < dims.width ? circ.x + radii : dims.width,
            circ.y + radii < dims.height ? circ.y + radii : dims.height));
    auto regout = cv::Rect(0, 30, regin.width, regin.height);
    auto derotin = cv::Rect(regin);
    auto derotout = cv::Rect(regin.width + 10, 30, derotin.width, derotin.height);
    auto outdims = cv::Size(regin.width + derotin.width + 10, 30 + std::max(regin.height, derotin.height));
    cv::Mat out_frame = cv::Mat::zeros(outdims, vid_frame.type());
    // ___ end TODO

    auto vidout = cv::VideoWriter(tmpfn, codec, fps, sideBySide ? outdims : dims);
    double cur_rotation = 0.0;
    double dtheta = -6.0 * rpm / fps;

    if (!vidout.isOpened())
        return(-2);

    std::ostringstream strrpm;
    strrpm.precision(2);
    strrpm << std::fixed << rpm;

    // construct overlay text
    std::string overlaytxtup = "Derotated at " + strrpm.str() + " rpm.";
    std::string overlaytxtlow = "Generated using diyrot.epss.ucla.edu";
    // calculate origin, white color
    auto origup = cv::Point(0, (int)vid.get(cv::CAP_PROP_FRAME_HEIGHT)-30);
    auto origlow = cv::Point(0, (int)vid.get(cv::CAP_PROP_FRAME_HEIGHT)-5);
    // use extra variables and overwrite previous for side-by-side
    auto origleft = cv::Point(out_frame.cols / 4 - 50, 20);
    auto origright = cv::Point(out_frame.cols * 3 / 4 - 60, 20);
    if (sideBySide) {
        origup = cv::Point(0, out_frame.rows-30);
        origlow = cv::Point(0, out_frame.rows-5);
    }
    auto textColor = cv::Scalar(255, 255, 255, 127.5);

    std::vector<unsigned char> adv_status;
    std::vector<float> adv_error;
    cv::Mat adv_frame_gray;
    cv::Mat adv_frame_gray_old;

    // LKpyr points. p0 is initialized by goodFeaturesToTrack, but
    // PointsHistory puts them into their own buckets. also restore the
    // viewport origin
    std::vector<cv::Point2f> adv_points;
    std::vector<cv::Point2f> adv_points_old;
    for (int i = 0; i < pth.points.size(); i++) {
        adv_points_old.push_back(pth.points[i].history.back() + circ);
    }

    // move the state a single frame ahead, since LKpyr needs 2 frames to begin,
    // and we already have the first set of points to track through GoodFeatures
    cur_rotation += dtheta;
    cv::cvtColor(vid_frame, adv_frame_gray_old, cv::COLOR_BGR2GRAY);
    vid.read(vid_frame);

    do {
        // TODO: convince myself t has the correct number of points
        // low priority, this seems fine
        pth.t.push_back(pth.t.size() / fps);
        cur_rotation += dtheta;
        cv::warpAffine(vid_frame, vid_frame, 
                       cv::getRotationMatrix2D(circ, cur_rotation, 1.0),
                       dims);
        cv::bitwise_and(vid_frame, 0, vid_frame, center_mask);
        cv::cvtColor(vid_frame, adv_frame_gray, cv::COLOR_BGR2GRAY);
        cv::calcOpticalFlowPyrLK(adv_frame_gray_old,
                                 adv_frame_gray,
                                 adv_points_old,
                                 adv_points,
                                 adv_status,
                                 adv_error,
                                 cv::Size(15, 15),
                                 2,
                                 cv::TermCriteria(
                                    cv::TermCriteria::COUNT+cv::TermCriteria::EPS,
                                    10,
                                    0.03));
        adv_frame_gray_old = adv_frame_gray.clone();

        // the assumed invariants here are that pth.points has all valid points
        // before non valid points and that adv_points_old has all the valid
        // points in the same order as pth.points. during each iteration,
        // adv_points, old, and status will always have the same count of points
        int points_demoted = 0;
        adv_points_old.clear();
        for (int j = 0; j < adv_points.size(); j++) {
            if (!adv_status[j]) {
                SinglePointHistory p = pth.points[j-points_demoted];
                pth.points.erase(pth.points.begin()+j-points_demoted);
                p.valid = false;
                pth.points.push_back(p);
                points_demoted++;
                continue;
            }
            // since we removed points_demoted entries in the loop now, the
            // matching index into pth will be points_demoted less than j.
            SinglePointHistory& cur_pth_point = pth.points[j-points_demoted];
            cur_pth_point.history.push_back(adv_points[j] - circ);

            // TODO: all the physics history recording
            //cur_pth_point.r.push_back(std::sqrt(std::pow(adv_points[j].x, 2), 
            //                                    std::pow(adv_points[j].y, 2)));
            
            // inserting is O(1) whereas remove and swap is O(n), so just insert
            adv_points_old.push_back(adv_points[j]);
        }

        for (int j = pth.t.size() - 1; j >= std::max(0, (int)pth.t.size() - WINDOW); j--) {
            for (int k = 0; k < adv_points.size()-points_demoted; k++) {
                cv::circle(vid_frame,
                           pth.points[k].history[j]+circ,
                           1,
                           pth.points[k].color,
                           -1);
            }
        }

        vidout << vid_frame;

        //if (!side_by_side) {
        //    cv::warpAffine(vid_frame, vid_frame, cv::getRotationMatrix2D(circ, i, 1.0), dims);
        //    cv::bitwise_and(vid_frame, 0, vid_frame, center_mask);
        //    cv::putText(vid_frame, overlaytxtup, origup, cv::FONT_HERSHEY_COMPLEX_SMALL, 1, color, 2, cv::LINE_AA);
        //    cv::putText(vid_frame, overlaytxtlow, origlow, cv::FONT_HERSHEY_COMPLEX_SMALL, 1, color, 2, cv::LINE_AA);
        //    vidout << vid_frame;
        // TODO: try and clean up the two cases, also see if advanced changes side by side
        //} else {
        //    vid_frame(regin).copyTo(out_frame(regout));
        //    cv::warpAffine(vid_frame, vid_frame, cv::getRotationMatrix2D(circ, i, 1.0), dims);
        //    cv::bitwise_and(vid_frame, 0, vid_frame, center_mask);
        //    vid_frame(derotin).copyTo(out_frame(derotout));
        //    cv::putText(out_frame, overlaytxtup, origup, cv::FONT_HERSHEY_COMPLEX_SMALL, 1, color, 2, cv::LINE_AA);
        //    cv::putText(out_frame, overlaytxtlow, origlow, cv::FONT_HERSHEY_COMPLEX_SMALL, 1, color, 2, cv::LINE_AA);
        //    cv::putText(out_frame, "Original", origleft, cv::FONT_HERSHEY_COMPLEX_SMALL, 1, color, 1, cv::LINE_AA);
        //    cv::putText(out_frame, "Derotated", origright, cv::FONT_HERSHEY_COMPLEX_SMALL, 1, color, 1, cv::LINE_AA);
        //    vidout << out_frame;
        //}
    } while(vid.read(vid_frame));

    vidout.release();
    rename(tmpfn.c_str(), outfn.c_str());
    return EXIT_SUCCESS;
}
