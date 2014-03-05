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

static void OnMouse(int event, int x, int y, int, void*) {
  if (event == EVENT_LBUTTONDOWN) {
    drawing = true;
    draw_mode = DRAW_FG;
  } else if (event == EVENT_RBUTTONDOWN) {
    drawing = true;
    draw_mode = DRAW_BG;
  } else if (event == EVENT_MOUSEMOVE && drawing) {
    if (draw_mode == DRAW_FG) {
      circle(fg_layer, Point(x, y), 5, Scalar::all(255), -1);
    } else {
      circle(bg_layer, Point(x, y), 5, Scalar::all(255), -1);
    }
  } else if (event == EVENT_LBUTTONUP || event == EVENT_RBUTTONUP) {
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

  cout << "Instructions : " << endl;
  cout << "- left click to draw foreground scribbles" << endl;
  cout << "- right click to draw background scribbles" << endl;

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

    const int key = cv::waitKey(1);
    if (key == 27) {
      break;
    }
  }
}
