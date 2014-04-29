// SimpleMatter demo
#include <iostream>

#include <glog/logging.h>

#include <chrono>

#include <boost/scoped_ptr.hpp>
#include <boost/scoped_array.hpp>

#include "cvutils.h"

#include "api.h"

using boost::scoped_ptr;
using boost::scoped_array;
using namespace std;
using namespace cv;
using namespace std::chrono;

Mat fg_layer, bg_layer;

enum DrawMode {
  DRAW_BG,
  DRAW_FG
};

DrawMode draw_mode = DRAW_BG;
bool drawing = false;
scoped_ptr<SimpleMatter> matter;

bool changed = true;

// Used to track cursor movements between two MOUSEMOVE events. For example
// on OSX, it seems the MOUSEMOVE events have a somewhat important interval
// between them
int x_prev = -1, y_prev = -1;

const int SCRIBLE_RADIUS = 5;

static void DrawScribbleCircle(int x, int y) {
  const int radius = SCRIBLE_RADIUS;
  const int rr = radius*radius;
  for (int dx = -radius; dx <=radius; ++dx) {
    for (int dy = -radius; dy <=radius; ++dy) {
      if ((dx*dx + dy*dy) <= rr) {
        if (draw_mode == DRAW_FG) {
          fg_layer.at<uint8_t>(y + dy, x + dx) = 255;
        } else {
          bg_layer.at<uint8_t>(y + dy, x + dx) = 255;
        }
      }
    }
  }
}

static float Length(const int* vec) {
  return sqrt(vec[0]*vec[0] + vec[1]*vec[1]);
}

static void UpdateMatter() {
  LOG(INFO) << "-- Updating masks";
  // Opencv uses row-major like libseg
  CHECK(fg_layer.isContinuous());
  CHECK(bg_layer.isContinuous());
  matter->UpdateMasks(bg_layer.ptr<uint8_t>(), fg_layer.ptr<uint8_t>());
  changed = true;
  LOG(INFO) << "-- done";
}

static void OnMouse(int event, int x, int y, int, void*) {
  if (event == EVENT_LBUTTONDOWN) {
    drawing = true;
    draw_mode = DRAW_FG;
    x_prev = x;
    y_prev = y;
  } else if (event == EVENT_RBUTTONDOWN) {
    drawing = true;
    draw_mode = DRAW_BG;
    x_prev = x;
    y_prev = y;
  } else if (event == EVENT_MOUSEMOVE && drawing) {
    if (x_prev != -1 && y_prev != -1) {
      // Draw scribbles on a line between (x_prev, y_prev) and (x,y), spaced
      // by SCRIBBLE_RADIUS
      const int d[2] = { x - x_prev, y - y_prev };
      const float len = Length(d);
      const float nsteps = len / (float)SCRIBLE_RADIUS;
      if (nsteps > 0) {
        const float tstep = 1.0f / nsteps;
        for (float t = 0; t < 1.0f; t += tstep) {
          const int curr_x = (int)(x_prev + t * d[0]);
          const int curr_y = (int)(y_prev + t * d[1]);
          DrawScribbleCircle(curr_x, curr_y);
        }
      }
      x_prev = x;
      y_prev = y;
    }
  } else if (event == EVENT_LBUTTONUP || event == EVENT_RBUTTONUP) {
    UpdateMatter();
    drawing = false;
    x_prev = -1;
    y_prev = -1;
  }
}

int main(int argc, char** argv) {
  string imgname = "data/default_img.jpg";
  if (argc > 1) {
    imgname = argv[1];
  }

  Mat img = imread(imgname, CV_LOAD_IMAGE_COLOR);
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

  matter.reset(new SimpleMatter(lab_l.get(), lab_a.get(), lab_b.get(),
                                W, H));

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

  Mat result(img.rows, img.cols, img.type(), Scalar::all(0));

  while (true) {
    auto start = high_resolution_clock::now();
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
    if (changed) {
      LOG(INFO) << "Refreshing from matter";
      matter->GetForegroundLikelihood(fg_likelihood.get());
      matter->GetBackgroundLikelihood(bg_likelihood.get());
      matter->GetForegroundDist(fg_dist.get());
      matter->GetBackgroundDist(bg_dist.get());
      matter->GetForegroundMask(final_mask.get());
      changed = false;
    }
    ImageSC<double>(fg_likelihood_mat, "fg_likelihood", false, false);
    ImageSC<double>(bg_likelihood_mat, "bg_likelihood", false, false);

    ImageSC<double>(fg_dist_mat, "fg_dist", false, false);
    ImageSC<double>(bg_dist_mat, "bg_dist", false, false);

    ImageSC<uint8_t>(final_mask_mat, "final_mask", false, false);

    result.setTo(0);
    img.copyTo(result, final_mask_mat);
    ShowImage(result, "result", false);

    auto end = high_resolution_clock::now();
    //LOG(INFO) << "time to display : "
              //<< duration_cast<milliseconds>(end - start).count() / 1000.0;
    //LOG(INFO) << "waitKey";
    const int key = cv::waitKey(30);
    if (key == 27 || key == 'q') {
      break;
    } else if (key == (int)'r') {
      LOG(INFO) << "Reset";
      fg_layer.setTo(0);
      bg_layer.setTo(0);
      matter.reset(new SimpleMatter(lab_l.get(), lab_a.get(),
                                    lab_b.get(), W, H));
      UpdateMatter();
    }
  }
}
