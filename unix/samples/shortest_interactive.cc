// SimpleMatter demo
#include <iostream>

#include <glog/logging.h>

#include <chrono>

#include <boost/scoped_ptr.hpp>
#include <boost/scoped_array.hpp>

#include "cvutils.h"

#include "utils.h"
#include "kde.h"
#include "geodesic.h"
#include "matting.h"

using boost::scoped_ptr;
using boost::scoped_array;
using namespace std;
using namespace cv;
using namespace std::chrono;

scoped_ptr<uint8_t> fg_mask, bg_mask;
Mat fg_mask_mat, bg_mask_mat;

enum DrawMode {
  DRAW_BG,
  DRAW_FG
};

DrawMode draw_mode = DRAW_BG;
bool drawing = false;

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
          fg_mask_mat.at<uint8_t>(y + dy, x + dx) = 255;
        } else {
          bg_mask_mat.at<uint8_t>(y + dy, x + dx) = 255;
        }
      }
    }
  }
}

static float Length(const int* vec) {
  return sqrt(vec[0]*vec[0] + vec[1]*vec[1]);
}

static void DoDraw(int x, int y) {
  // Draw scribbles on a line between (x_prev, y_prev) and (x,y), spaced
  // by SCRIBBLE_RADIUS
  const int d[2] = { x - x_prev, y - y_prev };
  const float len = Length(d);
  const float nsteps = len / (float)SCRIBLE_RADIUS;
  const float tstep = (nsteps > 0) ? 1.0f / nsteps : 1.0f ;
  for (float t = 0; t < 1.0f; t += tstep) {
    const int curr_x = (int)(x_prev + t * d[0]);
    const int curr_y = (int)(y_prev + t * d[1]);
    DrawScribbleCircle(curr_x, curr_y);
  }
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
      DoDraw(x, y);
      x_prev = x;
      y_prev = y;
    }
  } else if (event == EVENT_LBUTTONUP || event == EVENT_RBUTTONUP) {
    if (x_prev != -1 && y_prev != -1) {
      DoDraw(x, y);
    }
    changed = true;
    drawing = false;
    x_prev = -1;
    y_prev = -1;
  }
}

int main(int argc, char** argv) {
  //string imgname = "data/default_img.jpg";
  string imgname = "data/front.png";
  if (argc > 1) {
    imgname = argv[1];
  }
  cout << "Instructions : " << endl;
  cout << "- left click to draw foreground scribbles" << endl;
  cout << "- right click to draw background scribbles" << endl;

  Mat img = imread(imgname, CV_LOAD_IMAGE_COLOR);
  CHECK(img.data);

  // Denoise
  medianBlur(img, img, 3);

  const int W = img.cols;
  const int H = img.rows;

  fg_mask.reset(new uint8_t[W*H]);
  fg_mask_mat = Mat(H, W, CV_8UC1, fg_mask.get());
  bg_mask.reset(new uint8_t[W*H]);
  bg_mask_mat = Mat(H, W, CV_8UC1, bg_mask.get());

  fg_mask_mat.setTo(0);
  bg_mask_mat.setTo(0);

  namedWindow("img", 0);
  setMouseCallback("img", OnMouse, 0);
  imshow("img", img);

  //namedWindow("matted", 0);

  // RGB -> lab
  scoped_array<uint8_t> img_lab(new uint8_t[W*H*3]);
  cv::Mat img_lab_mat(H, W, CV_8UC3, img_lab.get());

  //cv::cvtColor(img, img_lab_mat, CV_BGR2Lab);
  cv::cvtColor(img, img_lab_mat, CV_BGR2HSV);

  scoped_array<double> height(new double[W*H*3]);
  cv::Mat height_mat(H, W, CV_64FC3, height.get());
  img_lab_mat.convertTo(height_mat, CV_64FC3);
  cv::normalize(height_mat, height_mat, 0, 1, cv::NORM_MINMAX);

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

    disp_img.setTo(Scalar(0, 255, 0), fg_mask_mat);
    disp_img.setTo(Scalar(0, 0, 255), bg_mask_mat);
    {
      Mat layer_overlap;
      bitwise_and(fg_mask_mat, bg_mask_mat, layer_overlap);
      disp_img.setTo(Scalar(255, 105, 180), layer_overlap);
    }
    imshow("img", disp_img);

    if (changed) {
      GeodesicDistanceMap(fg_mask.get(), height.get(), 3, W, H, fg_dist.get());
      GeodesicDistanceMap(bg_mask.get(), height.get(), 3, W, H, bg_dist.get());

      final_mask_mat = fg_dist_mat < bg_dist_mat;
      changed = false;
    }

    // Show both distance maps with the same range
    double min1, min2, max1, max2;
    minMaxIdx(fg_dist_mat, &min1, &max1, NULL, NULL);
    minMaxIdx(bg_dist_mat, &min2, &max2, NULL, NULL);

    //cv::Scalar mean, std;
    //meanStdDev(fg_dist_mat, mean, std);
    //LOG(INFO) << "mean : " << mean(0) << ", std : " << std(0);

    const double Amin = min(min1, min2);
    const double Amax = max(max1, max2);
    //const double Amin = 0;
    //const double Amax = 0.005;

    //cv::imwrite("fg_dist.png", fg_dist_mat);

    ImageSC<double>(fg_dist_mat, "fg_dist", false, false, Amin, Amax);
    ImageSC<double>(bg_dist_mat, "bg_dist", false, false, Amin, Amax);

    ImageSC<uint8_t>(final_mask_mat, "final_mask", false, false);

    result.setTo(Scalar(255,0,0));
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
      bg_mask_mat.setTo(0);
      fg_mask_mat.setTo(0);
      changed = true;
    }
  }
}
