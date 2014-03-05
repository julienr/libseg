#ifndef _LIBMATTING_CVUTILS_H_
#define _LIBMATTING_CVUTILS_H_

#include <iostream>
#include <algorithm>

#include <opencv2/contrib/contrib.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/imgproc/imgproc.hpp>

void WaitForEsc();

void ShowImage(const cv::Mat& img,
               const std::string& window_name,
               bool wait_for_esc=true);


// Similar to matlab's imagesc
template<class T>
void ImageSC(const cv::Mat& img,
             const std::string& window_name,
             bool wait_for_esc=true,
             bool verbose=true) {
  using namespace std;
  float Amin = *min_element(img.begin<T>(), img.end<T>());
  float Amax = *max_element(img.begin<T>(), img.end<T>());
  if (verbose) {
    cout << "[ImageSC " << window_name << "] min = " << Amin
         << ", max = " << Amax << endl;
  }
  cv::Mat A_scaled = (img - Amin)/(Amax - Amin);
  //LOG(INFO) << "A_scaled max : " << *max_element(A_scaled.begin<T>(), A_scaled.end<T>());
  cv::Mat display;
  A_scaled.convertTo(display, CV_8UC1, 255.0, 0);
  cv::applyColorMap(display, display, cv::COLORMAP_JET);
  ShowImage(display, window_name, wait_for_esc);
}



#endif
