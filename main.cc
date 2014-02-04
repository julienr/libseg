#include <iostream>
#include <fstream>
#include <array>

#include <glog/logging.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <boost/scoped_ptr.hpp>
#include <boost/scoped_array.hpp>

#include "kde.h"

using boost::scoped_ptr;
using boost::scoped_array;
using namespace std;

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
               bool wait_for_esc=true) {
  cv::namedWindow(window_name, CV_WINDOW_NORMAL | CV_WINDOW_KEEPRATIO
                  | CV_GUI_EXPANDED);
  imshow(window_name, img);
  if (wait_for_esc) {
    WaitForEsc();
  }
}

void SaveVector(const string& filename, const vector<float>& v) {
  fstream f(filename.c_str(), fstream::out);
  for (size_t i = 0; i < v.size(); ++i) {
    f << v[i] << "\t";
  }
  f.close();
}

// Saves foreground and background density estimation to fname
// channels should be an array of 3 uint8_t W*H array containing channel data
// Those can then be displayed using the plot_densities.py script
void SaveForegroundBackgroundDensities(uint8_t** channels,
                                       const uint8_t* fg,
                                       const uint8_t* bg,
                                       int W,
                                       int H,
                                       bool median_filter,
                                       const string& fname) {
  // For each channel, foreground and background probabilities
  vector<vector<float>> fg_prob(3), bg_prob(3);
  for (int i = 0; i < 3; ++i) {
    ColorChannelKDE(channels[i], fg, W, H, median_filter, &fg_prob[i]);
    ColorChannelKDE(channels[i], bg, W, H, median_filter, &bg_prob[i]);
  }

  fstream f(fname.c_str(), fstream::out);
  for (int c = 0; c < 3; ++c) {
    for (int k = 0; k < 2; ++k) { // k == 0 => fg, k == 1 => bg
      const vector<float>& v = (k == 0) ? fg_prob[c] : bg_prob[c];
      for (size_t i = 0; i < v.size(); ++i) {
        f << v[i] << "\t";
      }
      f << endl;
    }
  }
  f.close();
}

int main(int argc, char** argv) {
  // Load input image
  cv::Mat img = cv::imread(
      "data/alphamatting.com/input_training_lowres/GT18.png",
      CV_LOAD_IMAGE_COLOR);
  CHECK(img.data);

  // Load scribbles and binarize them such that pixels drawn by the user
  // have a value of 255
  cv::Mat scribble_fg = cv::imread(
      "data/alphamatting.com/GT18_FG.png",
      CV_LOAD_IMAGE_GRAYSCALE);
  CHECK(scribble_fg.data);
  cv::threshold(scribble_fg, scribble_fg, 1, 255, cv::THRESH_BINARY_INV);

  cv::Mat scribble_bg = cv::imread(
      "data/alphamatting.com/GT18_BG.png",
      CV_LOAD_IMAGE_GRAYSCALE);
  CHECK(scribble_bg.data);
  cv::threshold(scribble_bg, scribble_bg, 1, 255, cv::THRESH_BINARY_INV);

  cv::Mat img_lab;
  cv::cvtColor(img, img_lab, CV_BGR2Lab);

  // Split the image channels, getting a direct pointer to memory
  const int W = img.cols;
  const int H = img.rows;
  CHECK_EQ(scribble_fg.cols, W);
  CHECK_EQ(scribble_fg.rows, H);
  CHECK_EQ(scribble_bg.cols, W);
  CHECK_EQ(scribble_bg.rows, H);

  // Note: OpenCV's Mat uses row major storage
  vector<cv::Mat> lab;
  scoped_array<uint8_t> lab_l(new uint8_t[W*H]);
  lab.push_back(cv::Mat(H, W, CV_8U, lab_l.get()));
  scoped_array<uint8_t> lab_a(new uint8_t[W*H]);
  lab.push_back(cv::Mat(H, W, CV_8U, lab_a.get()));
  scoped_array<uint8_t> lab_b(new uint8_t[W*H]);
  lab.push_back(cv::Mat(H, W, CV_8U, lab_b.get()));
  cv::split(img_lab, lab);

  scoped_array<uint8_t> fg(new uint8_t[W*H]);
  {
    cv::Mat fg_mat(H, W, CV_8U, fg.get());
    scribble_fg.copyTo(fg_mat);
  }
  scoped_array<uint8_t> bg(new uint8_t[W*H]);
  {
    cv::Mat bg_mat(H, W, CV_8U, bg.get());
    scribble_bg.copyTo(bg_mat);
  }

  // Export densities for plot_densities.py script
  if (true) {
    uint8_t* channels[3] = {lab_l.get(), lab_a.get(), lab_b.get()};
    SaveForegroundBackgroundDensities(channels, fg.get(), bg.get(), W, H,
        false, "densities_nomedfilter.txt");
    SaveForegroundBackgroundDensities(channels, fg.get(), bg.get(), W, H,
        true, "densities_medfilter.txt");
  }

  ShowImage(lab[0], "lab[0]", true);

  //ShowImage(img_lab, "img_lab", false);
  //ShowImage(scribble_fg, "scribble fg", false);
  //ShowImage(scribble_bg, "scribble bg", true);
  return 0;
}
