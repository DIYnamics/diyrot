// input: FILE x y i_r o_r
// calls goodFeaturesToTrack on a masked image and serializes the result
// ./auto_good_features FILE X Y inner_r outer_r
int main(int argc, const char* argv[]) {
    // fail if not enough arguments
    if (argc < 5)
        return(-1);
    // extract filename from first argumnent
    const char* filename = argv[1];
    // get circle center x y as well as radii
    auto circ = cv::Point2f(strtod(argv[2], NULL), strtod(argv[3], NULL));
    double inner_radius = strtod(argv[4], NULL);
    double outer_radius = strtod(argv[5], NULL);
    // open video
    auto vid = cv::VideoCapture(filename);

    // get video size, store in Size object.
    auto dims = cv::Size(vid.get(cv::CAP_PROP_FRAME_WIDTH), vid.get(cv::CAP_PROP_FRAME_HEIGHT));
    // set up circle of interest masking. 8UC1 means 8-bit unsigned, 1 channel; this typing fits
    // the bitwise_and() function, but is in effect a 0 / non-0 binary mask.
    cv::Mat center_mask = cv::Mat::zeros(dims, CV_8UC1);
    // -1 means fill inside of circle, instead of drawing a circular curve of some width
    cv::circle(center_mask, circ, outer_radius, 255, -1);
    // invert the mask; we want to bitwise and 0 (zero out) portions of the image that are 
    // not what we are interested in.
    cv::bitwise_not(center_mask, center_mask);
    cv::circle(center_mask, circ, inner_radius, 255, -1);

    cv::Mat frame;
    // extract video frame, or fail
    if (!vid.read(frame))
        return(-2);

    // convert read video frame to grayscale
    cv::Mat frame_gray;
    cv::cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);

    // make everything outside outer mask black, circle inside inner mask black as well
    cv::bitwise_and(frame_gray, 0, frame_gray, center_mask);

    std::vector<cv::Point2f> output_points;
    cv::goodFeaturesToTrack(frame_gray, output_points, 100, 0.3, 7);

    for (auto p : output_points)
        std::cout << p.x << "," << p.y << ",";
    std::cout << std::endl;

    // close video, exit
    vid.release();
    return EXIT_SUCCESS;
}
