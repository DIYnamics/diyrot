#include <algorithm>
#include <opencv2/opencv.hpp>

#ifndef _LAYOUTUTILS_H_
#define _LAYOUTUTILS_H_

#define FONT cv::FONT_HERSHEY_COMPLEX
#define FONT_SCALE 2
#define FONT_THICK 4
#define LINE cv::LINE_AA
#define TEXT_BASE_HEIGHT 13.0
#define ATTR_TEXT_HEIGHT 27.0
#define ATTR_VID_HEIGHT 800.0 // declare float to avoid int division
#define TEXT_SLOPE ((ATTR_TEXT_HEIGHT - TEXT_BASE_HEIGHT) / ATTR_VID_HEIGHT)
#define COLOR_RED cv::Scalar(0, 0, 255, 200) // BGRA
#define COLOR_WHITE cv::Scalar(255, 255, 255, 200)
#define COLOR_BLUE cv::Scalar(255, 0, 0, 200)

const std::string kAttrString =
#if defined(PREVIEW)
    "PREVIEW VIDEO. Proceed filling out form to fully process.";
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

//NOLINTNEXTLINE(misc-definitions-in-headers)
cv::Size getStringSize(std::string input, int* baseline_return = NULL) {
    int baseline = 0;
    cv::Size text_size = 
        cv::getTextSize(input, FONT, FONT_SCALE, FONT_THICK, &baseline);
    if (baseline_return != NULL)
        *baseline_return = baseline;
    return cv::Size(text_size.width, text_size.height + (2 * baseline));
}

//NOLINTNEXTLINE(misc-definitions-in-headers)
cv::Mat drawString(std::string input, double q, int cv_mat_type) {
   const auto kColor =
#if defined(PREVIEW)
    COLOR_RED;
#else
    COLOR_WHITE;
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

//NOLINTNEXTLINE(misc-definitions-in-headers)
std::string makeInfoString(double rpm) {
    std::ostringstream rpm_string;
    rpm_string.precision(2);
    rpm_string << std::fixed << rpm;
    return "Derotated at " + rpm_string.str() + " rpm.";
}

// returned double q is linear multiplier for drawn text dims
//NOLINTNEXTLINE(misc-definitions-in-headers)
double getQuotient(double text_w, double text_h, double video_w, double video_h) {
    double frame_h = (video_h * TEXT_SLOPE) + TEXT_BASE_HEIGHT;
    double frame_w = text_w * frame_h / video_h;
    return std::max(frame_h / text_h,
                    std::min(frame_w, video_w) / text_w);
}

//NOLINTNEXTLINE(misc-definitions-in-headers)
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
    int scaled_text_h = static_cast<int>(q * text_h);
    // TODO: the labels probably need to be scaled along with text using getQuotient
    //double og_label_orig_w = roi.width * 0.5 - (getStringSize(kSbsOrigLabel).width / 2);
    //double derot_label_orig_w = roi.width * 1.5 - (getStringSize(kSbsDerotLabel).width / 2);
    double og_label_orig_w = 0;
    double derot_label_orig_w = 2 * roi.width - (q*getStringSize(kSbsDerotLabel).width);
    // quotient is ratio between frame and video
    if (roi.width > 500) {
        // fill with one line left, right justified layout
        return LayoutInfo {
            q,                                              // quotient
            cv::Size(2 * roi.width, roi.height + scaled_text_h), // output_frame_size
            cv::Point(roi.width, scaled_text_h),                 // derot_frame_origin
            cv::Point(0, 0),                                // info_text_origin
            cv::Point(2 * roi.width - attr_size.width * q, 0),       // attr_text_origin
            cv::Point(0, scaled_text_h),                         // og_frame_origin
            cv::Point(og_label_orig_w, scaled_text_h),           // og_label_origin
            cv::Point(derot_label_orig_w, scaled_text_h),        // derot_label_origin
        };
    } else {
        double q = getQuotient(std::max(attr_size.width, info_size.width),
                               text_h, 2 * roi.width, roi.height);
        // fill with sandwich layout like below
        return LayoutInfo {
            q,                                                  // quotient
            cv::Size(2 * roi.width, roi.height + 2 * scaled_text_h), // output_frame_size
            cv::Point(roi.width, scaled_text_h),                     // derot_frame_origin
            cv::Point(0, 0),                                    // info_text_origin 
            cv::Point(0, scaled_text_h + roi.height),                // attr_text_origin
            cv::Point(0, scaled_text_h),                             // og_frame_origin
            cv::Point(og_label_orig_w, scaled_text_h),               // og_label_origin
            cv::Point(derot_label_orig_w, scaled_text_h),            // derot_label_origin
        };
    }
#else
    // single video derot; this layout always puts info on top and attr on
    // bottom, left justified, same size
    auto text_w = std::max(attr_size.width, info_size.width);
    auto text_h = std::max(attr_size.height, info_size.height);
    double q = getQuotient(text_w, text_h, roi.width, roi.height);
    int scaled_text_h = static_cast<int>(q * text_h);
    int output_width = std::max(roi.width, static_cast<int>(q * text_w));
    double scaled_w = q * text_w;
    return LayoutInfo {
        q,                                                                           // quotient
        cv::Size(output_width, roi.height + (2 * scaled_text_h)),                    // output_frame_size
        cv::Point(0, scaled_text_h),                                                 // derot_frame_origin
        cv::Point(0, 0),                                                             // info_text_origin
        cv::Point(0, roi.height + scaled_text_h),                                    // attr_text_origin 
    };
#endif
}

#endif
