// Shortest-path matting example
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

#include <numeric>

#include "utils.h"
#include "kde.h"
#include "geodesic.h"
#include "matting.h"

#include "cvutils.h"

#include <nanoflann/nanoflann.hpp>
#include <nanoflann/vector_of_vector.hpp>

using boost::scoped_ptr;
using boost::scoped_array;
using namespace std;

int main(int argc, char** argv) {
  //const string imgname = "data/alphamatting.com/GT18";
  //const string imgname = "data/front";
  //const string imgname = "data/cat";
  const string imgname = "data/front2";

  // Load input image
  cv::Mat img = cv::imread(imgname + ".png", CV_LOAD_IMAGE_COLOR);
  CHECK(img.data);

  // Denoise
  medianBlur(img, img, 3);

  // Load scribbles and binarize them such that pixels drawn by the user
  // have a value of 255
  cv::Mat scribble_fg = cv::imread(imgname + "_FG.png",
                                   CV_LOAD_IMAGE_GRAYSCALE);
  CHECK(scribble_fg.data);
  cv::threshold(scribble_fg, scribble_fg, 1, 255, cv::THRESH_BINARY_INV);

  cv::Mat scribble_bg = cv::imread(imgname + "_BG.png",
                                   CV_LOAD_IMAGE_GRAYSCALE);
  CHECK(scribble_bg.data);
  cv::threshold(scribble_bg, scribble_bg, 1, 255, cv::THRESH_BINARY_INV);

  const int W = img.cols;
  const int H = img.rows;

  scoped_array<uint8_t> img_lab(new uint8_t[W*H*3]);
  cv::Mat img_lab_mat(H, W, CV_8UC3, img_lab.get());
  // TODO: HSV seems to work much better
  //img.copyTo(img_lab_mat);
  //cv::cvtColor(img, img_lab_mat, CV_BGR2HSV);
  //cv::cvtColor(img, img_lab_mat, CV_BGR2Lab);
  cv::cvtColor(img, img_lab_mat, CV_BGR2RGB);

  CHECK_EQ(scribble_fg.cols, W);
  CHECK_EQ(scribble_fg.rows, H);
  CHECK_EQ(scribble_bg.cols, W);
  CHECK_EQ(scribble_bg.rows, H);

  scoped_array<double> height(new double[W*H*3]);
  cv::Mat height_mat(H, W, CV_64FC3, height.get());
  img_lab_mat.convertTo(height_mat, CV_64FC3);
  cv::normalize(height_mat, height_mat, 0, 1, cv::NORM_MINMAX);

  ShowImage(img_lab_mat, "img_lab", false);
  ShowImage(height_mat, "height", false);
  //WaitForEsc();
  //return -1;

  LOG(INFO) << "W = " << W << ", H = " << H;

  // Note: OpenCV's Mat uses row major storage
  scoped_array<uint8_t> fg(new uint8_t[W*H]);
  cv::Mat fg_mask(H, W, CV_8U, fg.get());
  scribble_fg.copyTo(fg_mask);
  scoped_array<uint8_t> bg(new uint8_t[W*H]);
  cv::Mat bg_mask(H, W, CV_8U, bg.get());
  scribble_bg.copyTo(bg_mask);

  ShowImage(scribble_fg, "scribble fg", false);
  ShowImage(scribble_bg, "scribble bg", false);

  // Implement alternate matting technique where we just look for
  // the shortest path to a scribble and the distance between two pixels
  // is given by the euclidean distance (or cosine) between their colors.
  // This should put a lot of emphasis on scribbles locality, but that's good
  // because it's usually pretty easy for the user to just draw a scribble
  // around the person

  // Distance maps
  scoped_array<double> fg_dist(new double[W*H]);
  scoped_array<double> bg_dist(new double[W*H]);
  cv::Mat fg_dist_mat(H, W, CV_64F, fg_dist.get());
  cv::Mat bg_dist_mat(H, W, CV_64F, bg_dist.get());
  CHECK(scribble_fg.isContinuous());
  CHECK(scribble_bg.isContinuous());
  GeodesicDistanceMap(scribble_fg.ptr<uint8_t>(0), height.get(), 3, W, H,
                      fg_dist.get());
  GeodesicDistanceMap(scribble_bg.ptr<uint8_t>(0), height.get(), 3, W, H,
                      bg_dist.get());

  // Show both distance maps with the same range
  double min1, min2, max1, max2;
  minMaxIdx(fg_dist_mat, &min1, &max1, NULL, NULL);
  minMaxIdx(bg_dist_mat, &min2, &max2, NULL, NULL);

  cv::Scalar mean, std;
  meanStdDev(fg_dist_mat, mean, std);
  LOG(INFO) << "mean : " << mean(0) << ", std : " << std(0);

  const double Amin = min(min1, min2);
  const double Amax = max(max1, max2);
  //const double Amin = 0;
  //const double Amax = 0.005;

  //cv::imwrite("fg_dist.png", fg_dist_mat);

  ImageSC<double>(fg_dist_mat, "fg_dist", false, true, Amin, Amax);
  ImageSC<double>(bg_dist_mat, "bg_dist", false, true, Amin, Amax);

  // Segmentation
  cv::Mat fgmask = fg_dist_mat < bg_dist_mat;
  CHECK_EQ(fgmask.type(), CV_8U);
  ImageSC<uint8_t>(fgmask, "fgmask", false);

  //cv::imwrite("fgmask.png", fgmask);

  //ShowImage(lab[0], "lab[0]", true);

  cv::Mat result;
  img.copyTo(result, fgmask);
  result.setTo(cv::Scalar(255,255,255), ~fgmask);
  ShowImage(result, "result", false);
  ShowImage(img, "image", true);
  return 0;
}
