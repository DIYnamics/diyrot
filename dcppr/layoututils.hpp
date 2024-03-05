
// something to note re 30 pixel gap (which also needs to be changed based
// on video resolution: the black 30 pixels make the derotated view with
// its black mask look lopsided.
//
// position text on the screen 
// there are four positions: two for sideBySide only that say 'original' and
// 'derotated', and two that say something about rpm, info, etc the sidebysides
// should be right justified to the edges of their ROI on top try to put one
// line of text in the left side; if not, put one on bottom in the general
// case, put both on bottom, possibly in one line todo: to think: if someone
// uploads a 4k video, the rendered size of the text will be half of what it is
// for 2k; this is a little awkward conversly, if someone has a low res video,
// the text becomes too big
//

/*
 * 1. given a string of text, i can generate a w, h which opencv would render it at
 * 2. i am given the size of the output frame
 * 3. i would like the text to be laid out correctly..
 *  - i.e. the logical size of the text on screen stays the same
 *  - generally, this means text size linearly scales with frame size
 *  - however, this requires a single base point b, and a defined k
 *  - the implicit scale here is that the size of the string is also the size of the video, and k is 1
 *  - this means that whatever you create will _always_ exactly fill up the screen
 *      - this is definitely not true
 *      - algo seems to be: take both ratios, q_w and q_h of text vs video; q_w may take portion of video width
 *          1. try to scale according to height first, clamping lowest
 *          2. if scale would give text width larger than specified, then scale entire to 1 OR: line break
 * 4. you __need__ to have a ground truth range for how relative size of text
 *  - if it is lower than relative size, need to increase size
 *  - if it is higher than relative size, need to decrease / line break
 * 5. I'd like to do this at the very end
 *
 */
#include <algorithm>
#include <opencv2/opencv.hpp>

#ifndef _LAYOUTUTILS_H_
#define _LAYOUTUTILS_H_

#define FONT cv::FONT_HERSHEY_COMPLEX
#define FONT_SCALE 2
#define FONT_THICK 4
#define LINE cv::LINE_AA
#define TEXT_BASE_HEIGHT 15
#define ATTR_TEXT_HEIGHT 27
#define ATTR_VID_HEIGHT 1200
#define TEXT_SCALE ((ATTR_TEXT_HEIGHT - TEXT_BASE_HEIGHT) / ATTR_VID_HEIGHT)

const std::string kAttrString = 
#if defined(PREVIEW)
    "PREVIEW VIDEO. Proceed for full processing.";
#else
    "Generated using diyrot.epss.ucla.edu"; // this is 640 22, or 1279 43
                                            // for a 1200x1200 video, scale 1 looks ok
#endif
#if defined(SIDE_BY_SIDE)
const std::string kSbsOrigLabel = "Original";
const std::string kSbsDerotLabel = "Derotated";
#endif

typedef struct {
    double quotient;
    cv::Size outputFrameSize;
    cv::Point derotFrameOrigin;
    cv::Point infoTextOrigin; 
    cv::Point attrTextOrigin;
#if defined(SIDE_BY_SIDE)
    cv::Point ogFrameOrigin;
    cv::Point ogLabelOrigin;
    cv::Point derotLabelOrigin;
#endif
} LayoutInfo;

cv::Size getStringSize(std::string input, int* baseline_return = NULL) {
    int baseline = 0;
    cv::Size text_size = 
        cv::getTextSize(input, FONT, FONT_SCALE, FONT_THICK, &baseline);
    if (baseline_return != NULL)
        *baseline_return = baseline;
    return cv::Size(text_size.width, text_size.height + baseline);
}

cv::Mat drawString(std::string input, double q, int cv_mat_type) {
   const auto kColor =
#if defined(PREVIEW)
    cv::Scalar(255, 255, 255, 200);
#else
    cv::Scalar(255, 255, 255, 200);
#endif
    int baseline;
    auto text_dims = getStringSize(input, &baseline);
    auto text_mat = cv::Mat(text_dims, cv_mat_type);
    cv::putText(text_mat, input, cv::Point(0, text_dims.height - baseline),
                FONT, FONT_SCALE, kColor, FONT_THICK, LINE);
    auto dims = cv::Size(text_dims.width * q, text_dims.height * q);
    auto text_matrix = cv::Mat(dims, cv_mat_type);
    cv::resize(text_mat, text_matrix, dims);
    return text_matrix;
}

std::string makeInfoString(double rpm) {
    std::ostringstream rpm_string;
    rpm_string.precision(2);
    rpm_string << std::fixed << rpm;
    return "Derotated at " + rpm_string.str() + " rpm.";
}

double getQuotient(double text_w, double text_h, double video_w, double video_h) {
    double frame_h = (video_h * TEXT_SCALE) + TEXT_BASE_HEIGHT;
    double frame_w = text_w * frame_h / video_h;
    if (frame_w < video_w)
        return frame_h / text_h;
    else // clamp on exceeding width; scale may still be <1, must be smaller
        return video_w / text_w;
}

LayoutInfo makeLayout(cv::Size roi, double rpm) {
    // this layout tries to put attr, info at the same size on the same line,
    // left, right justified. If that doesn't work, info is on the top, then
    // videos, then attr on the bottom. info and attr have the same size, which
    // may be different from labels, which also have the same size.
    auto attr_size = getStringSize(kAttrString);
    auto info_size = getStringSize(makeInfoString(rpm));
#if defined(SIDE_BY_SIDE)
    auto text_w = (attr_size.width + info_size.width) * 1.05;
    auto text_h = std::max(attr_size.height, info_size.height);
    double q = getQuotient(text_w, text_h, 2 * roi.width, roi.height);
    double scaled_h = q * text_h;
    double og_label_orig_w = roi.width * 0.5 - (getStringSize(kSbsOrigLabel).width / 2);
    double derot_label_orig_w = roi.width * 1.5 - (getStringSize(kSbsDerotLabel).width / 2);
    if (q < 1) { // TODO: why is this <1 instead of >1???
                 // also: make sure this works with example_irl_tiny
        // fill with one line left, right justified layout
        return LayoutInfo {
            q,                                              // quotient
            cv::Size(2 * roi.width, roi.height + scaled_h), // output_frame_size
            cv::Point(roi.width, scaled_h),                 // derot_frame_origin
            cv::Point(0, 0),                                // info_text_origin
            cv::Point(info_size.width * q * 1.03, 0),       // attr_text_origin
            cv::Point(0, scaled_h),                         // og_frame_origin
            cv::Point(og_label_orig_w, scaled_h),           // og_label_origin
            cv::Point(derot_label_orig_w, scaled_h),        // derot_label_origin
        };
    } else {
        // fill with sandwich layout like below
        return LayoutInfo {
            q,                                                  // quotient
            cv::Size(2 * roi.width, roi.height + 2 * scaled_h), // output_frame_size
            cv::Point(roi.width, scaled_h),                     // derot_frame_origin
            cv::Point(0, 0),                                    // info_text_origin 
            cv::Point(0, scaled_h + roi.height),                // attr_text_origin
            cv::Point(0, scaled_h),                             // og_frame_origin
            cv::Point(og_label_orig_w, scaled_h),               // og_label_origin
            cv::Point(derot_label_orig_w, scaled_h),            // derot_label_origin
        };
    }
#else
    // single video derot; this layout always puts info on top and attr on
    // bottom, left justified, same size
    auto text_w = std::max(attr_size.width, info_size.width);
    auto text_h = std::max(attr_size.height, info_size.height);
    double q = getQuotient(text_w, text_h, roi.width, roi.height);
    double scaled_h = q * text_h;
    return LayoutInfo {
        q,                                                  // quotient
        cv::Size(roi.width, roi.height + (2 * scaled_h)),   // output_frame_size
        cv::Point(0, scaled_h),                             // derot_frame_origin
        cv::Point(0, 0),                                    // info_text_origin
        cv::Point(0, roi.height + scaled_h),                // attr_text_origin 
    };
#endif
}

#endif

