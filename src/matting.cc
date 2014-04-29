#include "matting.h"

#include <vector>
#include <limits>
#include <iostream>

#include <glog/logging.h>

#include "kde.h"
#include "geodesic.h"

using namespace std;

void ImageColorPDF(uint8_t const* const* channels,
                   const uint8_t* mask,
                   int W,
                   int H,
                   double* outimg) {
  vector<vector<double>> probs(3);
  for (int i = 0; i < 3; ++i) {
    ColorChannelKDE(channels[i], mask, W, H, true, &probs[i]);
  }

  ImageColorPDF(channels, probs, W, H, outimg);
}

void ImageColorPDF(uint8_t const* const* channels,
                   const std::vector<Scribble>& scribbles,
                   bool background,
                   int W,
                   int H,
                   double* outimg) {
  vector<vector<double>> probs(3);
  for (int i = 0; i < 3; ++i) {
    ColorChannelKDE(channels[i], scribbles, background, W, H, true, &probs[i]);
  }

  ImageColorPDF(channels, probs, W, H, outimg);
}

void ImageColorPDF(uint8_t const* const* channels,
                   const vector<vector<double>>& probs,
                   int W,
                   int H,
                   double* outimg) {
  double prob_max = numeric_limits<double>::min();
  for (int i = 0; i < W*H; ++i) {
    // The probabilities at a given value are very low (< 0.030), which is
    // mathematically correct (we have 255 such values and they sum to 1), but
    // can be problematic numerically. So we scale them by a factor of 10
    // TODO: Check that has no other incidence
    const double prob = probs[0][channels[0][i]]
                      * probs[1][channels[1][i]]
                      * probs[2][channels[2][i]];
    outimg[i] = prob;
    if (prob > prob_max) {
      prob_max = prob;
    }
  }
}

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

void FinalForegroundMask(const double* fg_dist,
                         const double* bg_dist,
                         int W,
                         int H,
                         uint8_t* outmask) {
  const int N = W*H;
  for (int i = 0; i < N; ++i) {
    outmask[i] = (fg_dist[i] < bg_dist[i]) ? 255 : 0;
  }
}
