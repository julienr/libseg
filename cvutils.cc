#include "cvutils.h"

void WaitForEsc() {
  while (1) {
    const int key = cv::waitKey(0);
    if (key == 27) { // escape
      break;
    }
  }
}

void ShowImage(const cv::Mat& img,
               const std::string& window_name,
               bool wait_for_esc) {
  cv::namedWindow(window_name, CV_WINDOW_NORMAL | CV_WINDOW_KEEPRATIO
                  | CV_GUI_EXPANDED);
  imshow(window_name, img);
  if (wait_for_esc) {
    WaitForEsc();
  }
}


