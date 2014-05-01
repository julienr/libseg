#include "matting.h"

#include <vector>
#include <limits>
#include <iostream>
#include <memory>

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

// 3D KDE (instead of per-channel like above)
void ImageColorPDF3(uint8_t const* const* channels,
                    const uint8_t* mask,
                    int W,
                    int H,
                    double* outimg) {
  int N = 0;
  for (int i = 0; i < W*H; ++i) {
    N += mask[i] != 0;
  }
  unique_ptr<double[]> xis(new double[N*3]);
  int curr = 0;
  for (int x = 0; x < W; ++x) {
    for (int y = 0; y < H; ++y) {
      const int cidx = y*W + x;
      if (mask[cidx]) {
        xis[curr + 0] = channels[0][cidx] / 255.0;
        xis[curr + 1] = channels[1][cidx] / 255.0;
        xis[curr + 2] = channels[2][cidx] / 255.0;
        curr += 3;
      }
    }
  }

  vector<double> weights(N, 1);

  // TODO: Should k-means cluster the image to drastically reduce number of xis
  unique_ptr<double[]> targets(new double[W*H*3]);
  for (int x = 0; x < W; ++x) {
    for (int y = 0; y < H; ++y) {
      const size_t cidx = y*W + x;
      const size_t idx = y*3*W + x*3;
      targets[idx + 0] = channels[0][cidx] / 255.0;
      targets[idx + 1] = channels[1][cidx] / 255.0;
      targets[idx + 2] = channels[2][cidx] / 255.0;
    }
  }

  FastKDE(3, N, W*H, xis.get(), weights.data(), targets.get(), outimg);
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
