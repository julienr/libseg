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

double Sum(const vector<double>& v) {
  double s = 0;
  for (double vv : v) {
    s += vv;
  }
  return s;
}

void ImageKNNDist(const cv::Mat& img, const cv::Mat& mask,
                  int W, int H, cv::Mat* result) {
  // First, apply KMeans to reduce number of points
  // http://docs.opencv.org/modules/core/doc/clustering.html
  vector<vector<double>> points;
  vector<vector<double>> centers;

  typedef vector<vector<double>> DataType;

  const int DIM = 3;

  CHECK_EQ(mask.type(), CV_8UC1);
  CHECK_EQ(img.type(), CV_8UC3);
  DataType samples;
  for (int x = 0; x < W; ++x) {
    for (int y = 0; y < H; ++y) {
      if (mask.at<uint8_t>(y, x)) {
        const cv::Vec3b& v = img.at<cv::Vec3b>(y, x);
        samples.push_back({(double)v[0]/255.0, (double)v[1]/255.0, (double)v[2]/255.0});
      }
    }
  }
  LOG(INFO) << "count : " << samples.size();

  typedef KDTreeVectorOfVectorsAdaptor<DataType, double>  MyKdTree;
  MyKdTree knn(DIM /*dim */, samples, 10 /* max leaf */);
  knn.index->buildIndex();

  const int K = 3;
  vector<size_t> indices(K);
  vector<double> dists(K);

  //nanoflann::KNNResultSet<double> result_set(K);
  //result_set.init(&indices[0], &dists[0]);

  vector<double> query_pt(DIM);
  for (int x = 0; x < W; ++x) {
    for (int y = 0; y < H; ++y) {
      const cv::Vec3b& v = img.at<cv::Vec3b>(y, x);
      query_pt = {(double)v[0]/255.0, (double)v[1]/255.0, (double)v[2]/255.0};
      // Find neighbors
      //knn.index->findNeighbors(result_set, &query_pt[0], nanoflann::SearchParams());
      knn.index->knnSearch(&query_pt[0], K, &indices[0], &dists[0]);
      const double sum_dist = Sum(dists) / (double)K;
      //result->at<double>(y, x) = exp(-sum_dist/(0.05*0.05));
      result->at<double>(y, x) = sum_dist;
    }
  }
  cv::normalize(*result, *result, 0, 1, cv::NORM_MINMAX);

  double min, max;
  minMaxIdx(*result, &min, &max, NULL, NULL);
  LOG(INFO) << "result min max : " << min << ", " << max;

  // TODO: This is weird, because we should do 1 - dist to get probability
  // from distance...
  //subtract(1.0, *result, *result);

}

/*void ImageKNNDist(const cv::Mat& img, const cv::Mat& mask,
                  int W, int H, cv::Mat* result) {
  CHECK_EQ(mask.type(), CV_8UC1);
  CHECK_EQ(img.type(), CV_8UC3);
  const int numFeatures = cv::countNonZero(mask);
  LOG(INFO) << "num features : " << numFeatures;
  cv::Mat features(numFeatures, 3, CV_32FC1);
  int fid = 0;
  for (int x = 0; x < W; ++x) {
    for (int y = 0; y < H; ++y) {
      if (mask.at<uint8_t>(y, x)) {
        const cv::Vec3b& v = img.at<cv::Vec3b>(y, x);
        features.at<float>(fid, 0) = v[0] / 255.0;
        features.at<float>(fid, 1) = v[1] / 255.0;
        features.at<float>(fid, 2) = v[2] / 255.0;

        //features.at<uint8_t>(fid, 0) = img.at<uint8_t>(y, x, 0);
        //features.at<uint8_t>(fid, 1) = img.at<uint8_t>(y, x, 1);
        //features.at<uint8_t>(fid, 2) = img.at<uint8_t>(y, x, 2);
        fid++;
      }
    }
  }
	// KdTree with 5 random trees
	cv::flann::KDTreeIndexParams indexParams(5);
  cv::flann::Index flann_index(features, indexParams);

  cv::Mat queries(W*H, 3, CV_32FC1);
  for (int x = 0; x < W; ++x) {
    for (int y = 0; y < H; ++y) {
      const cv::Vec3b& v = img.at<cv::Vec3b>(y, x);
      queries.at<float>(y*W+x, 0) = v[0] / 255.0;
      queries.at<float>(y*W+x, 1) = v[1] / 255.0;
      queries.at<float>(y*W+x, 2) = v[2] / 255.0;
    }
  }

  cv::Mat indices;
  cv::Mat dists;
  cv::flann::SearchParams params;
  const int K = 3;
  LOG(INFO) << "Starting knn search";
  flann_index.knnSearch(queries, indices, dists, 3, params);
  LOG(INFO) << "Done knn search";

  CHECK_EQ(result->type(), CV_64FC1);
  CHECK_EQ(dists.type(), CV_32FC1);

  for (int x = 0; x < W; ++x) {
    for (int y = 0; y < H; ++y) {
      result->at<double>(y, x) = dists.at<float>(y*W + x);
    }
  }
}*/

int main(int argc, char** argv) {
  //const string imgname = "data/alphamatting.com/GT18";
  const string imgname = "data/front";
  //const string imgname = "data/cat";

  // Load input image
  cv::Mat img = cv::imread(imgname + ".png", CV_LOAD_IMAGE_COLOR);
  CHECK(img.data);

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

  cv::Mat img_lab;
  // TODO: HSV seems to work much better
  //cv::cvtColor(img, img_lab, CV_BGR2HSV);
  //cv::cvtColor(img, img_lab, CV_BGR2Lab);
  cv::cvtColor(img, img_lab, CV_BGR2RGB);
  CHECK_EQ(img_lab.type(), CV_8UC3);

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
  cv::Mat fg_mask(H, W, CV_8U, fg.get());
  scribble_fg.copyTo(fg_mask);
  scoped_array<uint8_t> bg(new uint8_t[W*H]);
  cv::Mat bg_mask(H, W, CV_8U, bg.get());
  scribble_bg.copyTo(bg_mask);

  const uint8_t* channels[3] = {lab_l.get(), lab_a.get(), lab_b.get()};
  // Export densities for plot_densities.py script
  if (true) {
    SaveForegroundBackgroundDensities(channels, fg.get(), bg.get(), W, H,
        false, "densities_nomedfilter.txt");
    SaveForegroundBackgroundDensities(channels, fg.get(), bg.get(), W, H,
        true, "densities_medfilter.txt");
    LOG(INFO) << "Saved per-channel PDF to .txt";
  }

  ShowImage(scribble_fg, "scribble fg", false);
  ShowImage(scribble_bg, "scribble bg", false);

  // TODO: Implement alternate matting technique where we just look for
  // the shortest path to a scribble and the distance between two pixels
  // is given by the euclidean distance (or cosine) between their colors.
  // This should put a lot of emphasis on scribbles locality, but that's good
  // because it's usually pretty easy for the user to just draw a scribble
  // around the person

  // Color PDF
  scoped_array<double> fg_pdf(new double[W*H]);
  scoped_array<double> bg_pdf(new double[W*H]);
  cv::Mat fg_pdf_mat(H, W, CV_64F, fg_pdf.get());
  cv::Mat bg_pdf_mat(H, W, CV_64F, bg_pdf.get());

  //ImageColorPDF(channels, fg.get(), W, H, fg_pdf.get());
  //ImageColorPDF3(channels, fg.get(), W, H, fg_pdf.get());
  // Caution : at first, it might seem bg_pdf = 1 - fg_pdf, but this is not
  // the case
  //ImageColorPDF(channels, bg.get(), W, H, bg_pdf.get());
  //ImageColorPDF3(channels, bg.get(), W, H, bg_pdf.get());

  //{
    //cv::Mat _tmp1 = cv::imread(imgname + "_fg_dist.png", CV_LOAD_IMAGE_GRAYSCALE);
    //CHECK_EQ(_tmp1.type(), CV_8UC1);
    //double min, max;
    //minMaxIdx(_tmp1, &min, &max);
    //_tmp1.convertTo(fg_pdf_mat, CV_64F);
    ////cv::normalize(_tmp1, fg_pdf_mat, 0, 1, cv::NORM_MINMAX, CV_64F);
    ////ImageSC<double>(fg_pdf_mat, "fg_dist", true);
    //cv::Mat _tmp2 = cv::imread(imgname + "_bg_dist.png", CV_LOAD_IMAGE_GRAYSCALE);
    //_tmp2.convertTo(bg_pdf_mat, CV_64F);
  //}
  ImageKNNDist(img_lab, fg_mask, W, H, &bg_pdf_mat);
  ImageKNNDist(img_lab, bg_mask, W, H, &fg_pdf_mat);

  ImageSC<double>(fg_pdf_mat, "fg_pdf", false);
  ImageSC<double>(bg_pdf_mat, "bg_pdf", false);
  //WaitForEsc();
  //return EXIT_SUCCESS;

  // Foreground/Background likelihood
  scoped_array<double> fg_like(new double[W*H]);
  scoped_array<double> bg_like(new double[W*H]);
  cv::Mat fg_like_mat(H, W, CV_64F, fg_like.get());
  cv::Mat bg_like_mat(H, W, CV_64F, bg_like.get());
  ForegroundLikelihood(fg_pdf.get(), bg_pdf.get(), H, W, fg_like.get());
  ForegroundLikelihood(bg_pdf.get(), fg_pdf.get(), H, W, bg_like.get());

  //fg_like_mat = fg_pdf_mat;
  //bg_like_mat = bg_pdf_mat;
  //fg_like_mat.setTo(0.1);
  //bg_like_mat.setTo(0.1);

  ImageSC<double>(fg_like_mat, "fg_likelihood", false);
  ImageSC<double>(bg_like_mat, "bg_likelihood", false);


  // Distance maps
  scoped_array<double> fg_dist(new double[W*H]);
  scoped_array<double> bg_dist(new double[W*H]);
  cv::Mat fg_dist_mat(H, W, CV_64F, fg_dist.get());
  cv::Mat bg_dist_mat(H, W, CV_64F, bg_dist.get());
  CHECK(scribble_fg.isContinuous());
  CHECK(scribble_bg.isContinuous());
  GeodesicDistanceMap(scribble_fg.ptr<uint8_t>(0), fg_like.get(), W, H,
                      fg_dist.get());
  GeodesicDistanceMap(scribble_bg.ptr<uint8_t>(0), bg_like.get(), W, H,
                      bg_dist.get());

  // Show both distance maps with the same range
  double min1, min2, max1, max2;
  minMaxIdx(fg_dist_mat, &min1, &max1, NULL, NULL);
  minMaxIdx(bg_dist_mat, &min2, &max2, NULL, NULL);
  const double Amin = min(min1, min2);
  const double Amax = max(max1, max2);

  ImageSC<double>(fg_dist_mat, "fg_dist", false, true, Amin, Amax);
  ImageSC<double>(bg_dist_mat, "bg_dist", false, true, Amin, Amax);

  // Segmentation
  cv::Mat fgmask = fg_dist_mat < bg_dist_mat;
  CHECK_EQ(fgmask.type(), CV_8U);
  ImageSC<uint8_t>(fgmask, "fgmask", false);

  //ShowImage(lab[0], "lab[0]", true);

  cv::Mat result;
  img.copyTo(result, fgmask);
  result.setTo(cv::Scalar(255,255,255), ~fgmask);
  ShowImage(result, "result", false);
  ShowImage(img, "image", true);
  return 0;
}
