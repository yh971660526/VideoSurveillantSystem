﻿#ifndef THREEFRAMEDIFFERENCEBGS_H
#define THREEFRAMEDIFFERENCEBGS_H

#include <opencv2/opencv.hpp>
#include "IBGS.h"

class ThreeFrameDifferenceBGS : public IBGS
{
public:
    ThreeFrameDifferenceBGS();
    ~ThreeFrameDifferenceBGS();

    void process(const cv::Mat &img_input, cv::Mat &img_output, cv::Mat &img_bgmodel);

private:
    bool firstTime;
    cv::Mat img_input_prev1;
    cv::Mat img_input_prev;
    cv::Mat img_foreground;

    int threshold_diff1; //one frame different two frame threshold
    int threshold_diff2; //two frame different three frame threshold
    bool showOutput;

private:
    void saveConfig();
    void loadConfig();
};
#endif //THREEFRAMEDIFFERENCEBGS_H
