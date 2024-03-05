// main derotation program
#include <string>
#include <sstream>
#include <cstdio>
#include <errno.h>
#include <opencv2/opencv.hpp>


int main(int argc, const char* argv[]) {
    // set up 'avc1' codec, which translates to h264. we will wrap this in the mp4 container,
    // for the highest os/web browser compatibility.
    auto codec = cv::VideoWriter::fourcc('a', 'v', 'c', '1');

    // ./bin FILE X Y R RPM SBS OUTFILE
    // return 255 if not the case that all six arguments are present
    if (argc != 8)
        return(-1);
    // instantiate filename var, which is first argument in invokation
    std::string filename = std::string(argv[1]);
    // create cv float Point (NOT floating point) to represent center of circle of interest
    // we will mask (make black) everything outside of this circle
    auto circ = cv::Point2f(strtod(argv[2], NULL), strtod(argv[3], NULL));
    // radii of the circle of interest. int type bc need for cv::circle
    int radii = atoi(argv[4]);
    // rpm indicated. strtod is string to double - see c library documentation
    double rpm = strtod(argv[5], NULL);
    bool side_by_side = atoi(argv[6]); // any non-zero is true
    // instantiate output filename, last argument in invokaction
    // note that the container type is specified by the output filename
    // (we will always use .mp4)
    std::string outfn = std::string(argv[7]);
    std::string tmpfn = outfn;
    auto f = outfn.find_last_of("/");
    f = (f == std::string::npos) ? 0 : f+1;
    tmpfn.insert(f, "TMP");
    // problem with getting the rpm? or some input was too large to represent? return 255..
    if (!rpm || errno == ERANGE)
        return(-1);
    // create a VideoCapture object. openCV tries to open the video file.
    auto vid = cv::VideoCapture(filename);
    // create a matrix for storing a single video frame
    cv::Mat vid_frame;
    // if cannot read, return 254.
    if (!vid.read(vid_frame))
        return(-2);

    // fps can be non-integer! think as inverse of seconds-per-frame: frames are discrete, seconds
    // taken do not have to be.
    double fps = vid.get(cv::CAP_PROP_FPS);
    // get video size, store in Size object.
    auto dims = cv::Size(vid.get(cv::CAP_PROP_FRAME_WIDTH), vid.get(cv::CAP_PROP_FRAME_HEIGHT));
    // set up circle of interest masking. 8UC1 means 8-bit unsigned, 1 channel; this typing fits
    // the bitwise_and() function, but is in effect a 0 / non-0 binary mask.
    cv::Mat center_mask = cv::Mat::zeros(dims, CV_8UC1);
    // -1 means fill inside of circle, instead of drawing a circular curve of some width
    // this changes every pixel inside the circle of interest from 0 to 255
    cv::circle(center_mask, circ, radii, 255, -1);
    // invert the mask; we want to bitwise and 0 (zero out) portions of the image that are 
    // not what we are interested in.
    cv::bitwise_not(center_mask, center_mask);

    // calculate side by side variables, but don't necessarily use them
    auto regin = cv::Rect(
        cv::Point(circ.x - radii > 0 ? circ.x - radii : 0,
            circ.y - radii > 0 ? circ.y - radii : 0),
        cv::Point(circ.x + radii < dims.width ? circ.x + radii : dims.width,
            circ.y + radii < dims.height ? circ.y + radii : dims.height));
    auto regout = cv::Rect(0, 30, regin.width, regin.height);
    auto derotin = cv::Rect(regin);
    auto derotout = cv::Rect(regin.width + 10, 30, derotin.width, derotin.height);
    auto outdims = cv::Size(regin.width + derotin.width + 10, 30 + std::max(regin.height, derotin.height));
    // set up output frame to write to
    cv::Mat out_frame = cv::Mat::zeros(outdims, vid_frame.type());


    // open an output video using our output filename/container, avc codec, original video's fps,
    // and original video's dimensions. note that the fps may be slightly rounded between input video
    // and output video. I'm not sure why this is the case.
    // additionally, prepend 'TMP' to file name so that slow systems do not break
    // incremental angle change for each frame
    // create a different sized output depending on whether side by side
    auto vidout = cv::VideoWriter(tmpfn, codec, fps, side_by_side ? outdims : dims);
    double i = 0.0;
    // change in degree angle per frame.
    // explaination:
    // 360 * rpm => change in angles per min
    // 360 * rpm / 60 = 6*rpm => change in angles per second
    // 6 * rpm / fps => change in angles per frame.
    // negative sign is bc we are 'derotating' the video, or going opposite to the lab frame rpm.
    double dtheta = -6.0 * rpm / fps;

    // return 254 if output video cannot be opened, for whatever reason
    if (!vidout.isOpened())
        return(-2);

    // stringstream to manage rpm precision
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
    if (side_by_side) {
        origup = cv::Point(0, out_frame.rows-30);
        origlow = cv::Point(0, out_frame.rows-5);
    }
    auto color = cv::Scalar(255, 255, 255, 127.5);

    // main derotation loop. if vid.read() fails (likely due to end of file), stop loop.
    // otherwise each iteration of the do-while loop has a fresh video frame to derotate.
    do {
        // increase derotation angle for by amount for 1 frame
        i += dtheta;
        if (!side_by_side) {
            // warp in-place (reusing vid_frame), using a rotation matrix centered at circle of interest's
            // center, with rotation angle i, and no 'resizing' (1.0). specify size of vid_frame as expected.
            cv::warpAffine(vid_frame, vid_frame, cv::getRotationMatrix2D(circ, i, 1.0), dims);
            // bitwise and 0 (make black) vid_frame in place, but only for pixels where center_mask != 0.
            // this is every pixel outside the circle of interest.
            cv::bitwise_and(vid_frame, 0, vid_frame, center_mask);
            cv::putText(vid_frame, overlaytxtup, origup, cv::FONT_HERSHEY_COMPLEX_SMALL, 1, color, 2, cv::LINE_AA);
            cv::putText(vid_frame, overlaytxtlow, origlow, cv::FONT_HERSHEY_COMPLEX_SMALL, 1, color, 2, cv::LINE_AA);
            // write vid_frame to the video. equivalent to vidout.write(vid_frame)
            vidout << vid_frame;
        } else {
            vid_frame(regin).copyTo(out_frame(regout));
            cv::warpAffine(vid_frame, vid_frame, cv::getRotationMatrix2D(circ, i, 1.0), dims);
            cv::bitwise_and(vid_frame, 0, vid_frame, center_mask);
            vid_frame(derotin).copyTo(out_frame(derotout));
            cv::putText(out_frame, overlaytxtup, origup, cv::FONT_HERSHEY_COMPLEX_SMALL, 1, color, 2, cv::LINE_AA);
            cv::putText(out_frame, overlaytxtlow, origlow, cv::FONT_HERSHEY_COMPLEX_SMALL, 1, color, 2, cv::LINE_AA);
            cv::putText(out_frame, "Original", origleft, cv::FONT_HERSHEY_COMPLEX_SMALL, 1, color, 1, cv::LINE_AA);
            cv::putText(out_frame, "Derotated", origright, cv::FONT_HERSHEY_COMPLEX_SMALL, 1, color, 1, cv::LINE_AA);
            vidout << out_frame;
        }
    } while(vid.read(vid_frame));

    // finish processing the output video, and close the file
    vidout.release();
    // move back to original filename
    rename(tmpfn.c_str(), outfn.c_str());
    // exit success is cstdlib macro - apparently is 0
    return EXIT_SUCCESS;
}
