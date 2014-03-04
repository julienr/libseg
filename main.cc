#include <iostream>
#include <fstream>
#include <array>

#include <glog/logging.h>

#include <opencv2/contrib/contrib.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <boost/scoped_ptr.hpp>
#include <boost/scoped_array.hpp>

#include "utils.h"
#include "kde.h"
#include "geodesic.h"

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

// Similar to matlab's imagesc
template<class T>
void ImageSC(const cv::Mat& img,
             const std::string& window_name,
             bool wait_for_esc=true) {
  float Amin = *min_element(img.begin<T>(), img.end<T>());
  float Amax = *max_element(img.begin<T>(), img.end<T>());
  LOG(INFO) << "[ImageSC " << window_name << "] min = " << Amin
            << ", max = " << Amax;
  cv::Mat A_scaled = (img - Amin)/(Amax - Amin);
  //LOG(INFO) << "A_scaled max : " << *max_element(A_scaled.begin<T>(), A_scaled.end<T>());
  cv::Mat display;
  A_scaled.convertTo(display, CV_8UC1, 255.0, 0);
  cv::applyColorMap(display, display, cv::COLORMAP_JET);
  ShowImage(display, window_name, wait_for_esc);
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
void SaveForegroundBackgroundDensities(const uint8_t** channels,
                                       const uint8_t* fg,
                                       const uint8_t* bg,
                                       int W,
                                       int H,
                                       bool median_filter,
                                       const string& fname) {
  // For each channel, foreground and background probabilities
  vector<vector<double>> fg_prob(3), bg_prob(3);
  for (int i = 0; i < 3; ++i) {
    ColorChannelKDE(channels[i], fg, W, H, median_filter, &fg_prob[i]);
    ColorChannelKDE(channels[i], bg, W, H, median_filter, &bg_prob[i]);
  }

  fstream f(fname.c_str(), fstream::out);
  for (int c = 0; c < 3; ++c) {
    for (int k = 0; k < 2; ++k) { // k == 0 => fg, k == 1 => bg
      const vector<double>& v = (k == 0) ? fg_prob[c] : bg_prob[c];
      for (size_t i = 0; i < v.size(); ++i) {
        f << v[i] << "\t";
      }
      f << endl;
    }
  }
  f.close();
}

// Computes P(c_x | M) where M is background/foreground estimated by KDE from
// the mask. c_x is a pixel.
//
// outimg should be a user-allocated W*H image
void ImageColorPDF(const uint8_t** channels,
                   const uint8_t* mask,
                   int W,
                   int H,
                   double* outimg) {
  vector<vector<double>> probs(3);
  for (int i = 0; i < 3; ++i) {
    ColorChannelKDE(channels[i], mask, W, H, true, &probs[i]);
  }

  double prob_max = numeric_limits<double>::min();
  for (int i = 0; i < W*H; ++i) {
    // The probabilities at a given value are very low (< 0.030), which is
    // mathematically correct (we have 255 such values and they sum to 1), but
    // can be problematic numerically. So we scale them by a factor of 10
    // and then normalize the whole probability image to 0, 1
    const double prob = 10*probs[0][channels[0][i]]
                      * 10*probs[1][channels[1][i]]
                      * 10*probs[2][channels[2][i]];
    outimg[i] = prob;
    if (prob > prob_max) {
      prob_max = prob;
    }
  }

  // normalize
  // TODO: We shouldn't normalize here, because this will result in different
  // scaling for fg and bg probabilities
  //for (int i = 0; i < W*H; ++i) {
    //outimg[i] /= prob_max;
  //}
}

// Equation 1. of Bai09 :
// P_F(cx) = P(cx|F) / (P(cx|F) + P(cx|B))
//
// likelihood should be a user-allocated W*H image
void ForegroundLikelihood(const double* P_cx_F,
                          const double* P_cx_B,
                          int W,
                          int H,
                          double* likelihood) {
  for (int i = 0; i < W*H; ++i) {
    // Avoid division by zero
    if (P_cx_F[i] == 0 && P_cx_B[i] == 0) {
      // If P(cx|B) = 0, we have P(cx|F) / (P(cx|F), so set to 1
      likelihood[i] = 1;
    } else {
      likelihood[i] = P_cx_F[i] / (P_cx_F[i] + P_cx_B[i]);
    }
  }
}

int main(int argc, char** argv) {
  //const string imgname = "GT18";
  const string imgname = "GT06";

  // Load input image
  cv::Mat img = cv::imread(
      "data/alphamatting.com/input_training_lowres/" + imgname + ".png",
      CV_LOAD_IMAGE_COLOR);
  CHECK(img.data);

  // Load scribbles and binarize them such that pixels drawn by the user
  // have a value of 255
  cv::Mat scribble_fg = cv::imread(
      "data/alphamatting.com/" + imgname + "_FG.png",
      CV_LOAD_IMAGE_GRAYSCALE);
  CHECK(scribble_fg.data);
  cv::threshold(scribble_fg, scribble_fg, 1, 255, cv::THRESH_BINARY_INV);

  cv::Mat scribble_bg = cv::imread(
      "data/alphamatting.com/" + imgname + "_BG.png",
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

  LOG(INFO) << "W = " << W << ", H = " << H;

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

  const uint8_t* channels[3] = {lab_l.get(), lab_a.get(), lab_b.get()};
  // Export densities for plot_densities.py script
  if (false) {
    SaveForegroundBackgroundDensities(channels, fg.get(), bg.get(), W, H,
        false, "densities_nomedfilter.txt");
    SaveForegroundBackgroundDensities(channels, fg.get(), bg.get(), W, H,
        true, "densities_medfilter.txt");
  }

  ShowImage(scribble_fg, "scribble fg", false);
  ShowImage(scribble_bg, "scribble bg", false);

  // Color PDF
  scoped_array<double> fg_pdf(new double[W*H]);
  scoped_array<double> bg_pdf(new double[W*H]);
  ImageColorPDF(channels, fg.get(), W, H, fg_pdf.get());
  // Caution : at first, it might seem bg_pdf = 1 - fg_pdf, but this is not
  // the case
  ImageColorPDF(channels, bg.get(), W, H, bg_pdf.get());

  // Foreground/Background likelihood
  scoped_array<double> fg_prob(new double[W*H]);
  scoped_array<double> bg_prob(new double[W*H]);
  cv::Mat fg_prob_mat(H, W, CV_64F, fg_prob.get());
  cv::Mat bg_prob_mat(H, W, CV_64F, bg_prob.get());
  ForegroundLikelihood(fg_pdf.get(), bg_pdf.get(), H, W, fg_prob.get());
  ForegroundLikelihood(bg_pdf.get(), fg_pdf.get(), H, W, bg_prob.get());

  ImageSC<double>(fg_prob_mat, "fg_prob", false);
  ImageSC<double>(bg_prob_mat, "bg_prob", false);


  // Distance maps
  scoped_array<double> fg_dist(new double[W*H]);
  scoped_array<double> bg_dist(new double[W*H]);
  cv::Mat fg_dist_mat(H, W, CV_64F, fg_dist.get());
  cv::Mat bg_dist_mat(H, W, CV_64F, bg_dist.get());
  CHECK(scribble_fg.isContinuous());
  CHECK(scribble_bg.isContinuous());
  GeodesicDistanceMap(scribble_fg.ptr<uint8_t>(0), fg_prob.get(), W, H,
                      fg_dist.get());
  GeodesicDistanceMap(scribble_bg.ptr<uint8_t>(0), bg_prob.get(), W, H,
                      bg_dist.get());
  ImageSC<double>(fg_dist_mat, "fg_dist", false);
  ImageSC<double>(bg_dist_mat, "bg_dist", false);

  // Segmentation
  cv::Mat fgmask = fg_dist_mat < bg_dist_mat;
  CHECK_EQ(fgmask.type(), CV_8U);
  ImageSC<uint8_t>(fgmask, "fgmask", false);

  //ShowImage(lab[0], "lab[0]", true);

  cv::Mat result;
  img.copyTo(result, fgmask);
  ShowImage(result, "result", false);
  ShowImage(img, "image", true);
  return 0;
}
