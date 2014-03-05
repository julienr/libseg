#include <iostream>

#include <glog/logging.h>

#include <boost/scoped_ptr.hpp>
#include <boost/scoped_array.hpp>

#include "cvutils.h"

#include "api.h"

using boost::scoped_ptr;
using boost::scoped_array;
using namespace std;
using namespace cv;

Mat fg_layer, bg_layer;
enum DrawMode {
  DRAW_BG,
  DRAW_FG
};
DrawMode draw_mode = DRAW_BG;
bool drawing = false;
scoped_ptr<Scribble> current_scribble;

scoped_ptr<Matter> matter;

static void OnMouse(int event, int x, int y, int, void*) {
  if (event == EVENT_LBUTTONDOWN) {
    drawing = true;
    draw_mode = DRAW_FG;
    current_scribble.reset(new Scribble);
    current_scribble->background = false;
  } else if (event == EVENT_RBUTTONDOWN) {
    drawing = true;
    draw_mode = DRAW_BG;
    current_scribble.reset(new Scribble);
    current_scribble->background = true;
  } else if (event == EVENT_MOUSEMOVE && drawing) {
    const int radius = 5;
    const int rr = radius*radius;
    for (int dx = -radius; dx <=radius; ++dx) {
      for (int dy = -radius; dy <=radius; ++dy) {
        if ((dx*dx + dy*dy) <= rr) {
          current_scribble->pixels.push_back(::Point2i(x + dx, y + dy));
          if (draw_mode == DRAW_FG) {
            fg_layer.at<uint8_t>(y + dy, x + dx) = 255;
          } else {
            bg_layer.at<uint8_t>(y + dy, x + dx) = 255;
          }
        }
      }
    }
    //current_scribble->pixels.
    //if (draw_mode == DRAW_FG) {
      //circle(fg_layer, Point(x, y), radius, Scalar::all(255), -1);
    //} else {
      //circle(bg_layer, Point(x, y), radius, Scalar::all(255), -1);
    //}
  } else if (event == EVENT_LBUTTONUP || event == EVENT_RBUTTONUP) {
    LOG(INFO) << "-- Adding scribble";
    matter->AddScribble(*current_scribble);
    LOG(INFO) << "-- done";
    current_scribble.reset();
    drawing = false;
  }
}

int main(int argc, char** argv) {
  const string imgname = "GT18";

  Mat img = imread(
    "data/alphamatting.com/input_training_lowres/" + imgname + ".png",
    CV_LOAD_IMAGE_COLOR);
  CHECK(img.data);

  fg_layer = Mat::zeros(img.rows, img.cols, CV_8UC1);
  bg_layer = Mat::zeros(img.rows, img.cols, CV_8UC1);

  namedWindow("img", 0);
  setMouseCallback("img", OnMouse, 0);
  imshow("img", img);

  //namedWindow("matted", 0);

  cout << "Instructions : " << endl;
  cout << "- left click to draw foreground scribbles" << endl;
  cout << "- right click to draw background scribbles" << endl;

  const int W = img.cols;
  const int H = img.rows;

  // RGB -> lab
  cv::Mat img_lab;
  cv::cvtColor(img, img_lab, CV_BGR2Lab);
  vector<cv::Mat> lab;
  scoped_array<uint8_t> lab_l(new uint8_t[W*H]);
  lab.push_back(cv::Mat(H, W, CV_8U, lab_l.get()));
  scoped_array<uint8_t> lab_a(new uint8_t[W*H]);
  lab.push_back(cv::Mat(H, W, CV_8U, lab_a.get()));
  scoped_array<uint8_t> lab_b(new uint8_t[W*H]);
  lab.push_back(cv::Mat(H, W, CV_8U, lab_b.get()));
  cv::split(img_lab, lab);

  matter.reset(new Matter(lab_l.get(), lab_a.get(), lab_b.get(), W, H));

  scoped_array<double> fg_likelihood(new double[W*H]);
  cv::Mat fg_likelihood_mat(H, W, CV_64F, fg_likelihood.get());
  fg_likelihood_mat.setTo(0);
  scoped_array<double> bg_likelihood(new double[W*H]);
  cv::Mat bg_likelihood_mat(H, W, CV_64F, bg_likelihood.get());
  bg_likelihood_mat.setTo(0);

  scoped_array<double> fg_dist(new double[W*H]);
  Mat fg_dist_mat(H, W, CV_64F, fg_dist.get());
  fg_dist_mat.setTo(0);
  scoped_array<double> bg_dist(new double[W*H]);
  Mat bg_dist_mat(H, W, CV_64F, bg_dist.get());
  bg_dist_mat.setTo(0);

  scoped_array<uint8_t> final_mask(new uint8_t[W*H]);
  cv::Mat final_mask_mat(H, W, CV_8UC1, final_mask.get());
  final_mask_mat.setTo(0);

  int nscribbles = matter->NumScribbles();

  Mat result(img.rows, img.cols, img.type(), Scalar::all(0));

  while (true) {
    // http://opencv-python-tutroals.readthedocs.org/en/latest/py_tutorials/py_core/py_image_arithmetics/py_image_arithmetics.html
    // Display scribbles on top of images :
    // - fg scribbles are green
    // - bg scribbles are red
    // - fg and bg scribbles intersections (bad) are pink
    Mat disp_img;
    img.copyTo(disp_img);
    CHECK_EQ(disp_img.type(), CV_8UC3);

    disp_img.setTo(Scalar(0, 255, 0), fg_layer);
    disp_img.setTo(Scalar(0, 0, 255), bg_layer);
    {
      Mat layer_overlap;
      bitwise_and(fg_layer, bg_layer, layer_overlap);
      disp_img.setTo(Scalar(255, 105, 180), layer_overlap);
    }
    imshow("img", disp_img);

    // Refresh images from matter if something has changed
    if (matter->NumScribbles() != nscribbles) {
      LOG(INFO) << "Refreshing from matter";
      matter->GetForegroundLikelihood(fg_likelihood.get());
      matter->GetBackgroundLikelihood(bg_likelihood.get());
      matter->GetForegroundDist(fg_dist.get());
      matter->GetBackgroundDist(bg_dist.get());
      matter->GetForegroundMask(final_mask.get());
      nscribbles = matter->NumScribbles();
    }
    ImageSC<double>(fg_likelihood_mat, "fg_likelihood", false, false);
    ImageSC<double>(bg_likelihood_mat, "bg_likelihood", false, false);

    ImageSC<double>(fg_dist_mat, "fg_dist", false, false);
    ImageSC<double>(bg_dist_mat, "bg_dist", false, false);

    ImageSC<uint8_t>(final_mask_mat, "final_mask", false, false);

    img.copyTo(result, final_mask_mat);
    ShowImage(result, "result", false);

    const int key = cv::waitKey(1);
    if (key == 27) {
      break;
    }
  }
}
