#include "kde.h"
#include <cmath>
#include <algorithm>
#include <iostream>

#include <glog/logging.h>

#include <figtree.h>

using namespace std;

const double NORMAL_FACTOR = 1/(double)sqrt(2*M_PI);

double GaussianKernel(double t, double xi, double h) {
  const double x = (t - xi) / h;
  // TODO: We have to remove NORMAL_FACTOR so that is sums to 1
  // (see plot_densities.py). Is that normal ?

  //return NORMAL_FACTOR * exp(-0.5 * x * x);
  return exp(-0.5 * x * x);
}

// Use Scott's Rule, like scipy :
//   h = n**(-1./(d+4))
// where n is the number of data points, d the number of dimensions
// http://docs.scipy.org/doc/scipy/reference/generated/scipy.stats.gaussian_kde.html
double EstimateBandwidth(int ndata, int ndims) {
  return pow(ndata, -1/(double)(ndims + 4));
}

void UnivariateKDE(const vector<double>& xis,
                   const vector<double>& weights,
                   const vector<double>& targets,
                   vector<double>* target_prob) {
  CHECK_EQ(xis.size(), weights.size());
  const double h = EstimateBandwidth(xis.size(), 1);
  LOG(INFO) << "h : " << h;
  for (size_t ti = 0; ti < targets.size(); ++ti) {
    double prob = 0;
    for (size_t i = 0; i < xis.size(); ++i) {
      prob += weights[i] * GaussianKernel(targets[ti], xis[i], h);
    }
    target_prob->push_back(prob);
  }
}

void FastUnivariateKDE(const std::vector<double>& xis,
                       const std::vector<double>& weights,
                       const std::vector<double>& targets,
                       std::vector<double>* target_prob,
                       double epsilon) {
  // Figtree's h is not exactly the same as standard deviation :
  // (from figtree sample.cpp)
  // The bandwidth.  NOTE: this is not the same as standard deviation since
  // the Gauss Transform sums terms exp( -||x_i - y_j||^2 / h^2 ) as opposed
  // to  exp( -||x_i - y_j||^2 / (2*sigma^2) ).  Thus, if sigma is known,
  // bandwidth can be set to h = sqrt(2)*sigma.
  const double h = sqrt(2) * EstimateBandwidth(xis.size(), 1);
  LOG(INFO) << "h : " << h;
  const int d = 1;
  const int N = xis.size();
  const int M = targets.size();
  const int W = 1;

  target_prob->resize(M);

  figtree(d, N, M, W,
          const_cast<double*>(xis.data()),
          h,
          const_cast<double*>(weights.data()),
          const_cast<double*>(targets.data()),
          epsilon,
          target_prob->data());
}

// Return the median of the elements in v. Note that v WILL be modified
template<class T>
T Median(vector<T>* v) {
  // TODO: This is not completely correct. If v has an even number of elements,
  // the median should be the average of the middle two
  // http://stackoverflow.com/questions/1719070/what-is-the-right-approach-when-using-stl-container-for-median-calculation
  size_t n = v->size() / 2;
  std::nth_element(v->begin(), v->begin() + n, v->end());
  return v->at(n);
}

void MedianFilter(const vector<double>& v,
                  size_t hwsize, // half window size
                  vector<double>* vfilt) {
  for (size_t i = 0; i < v.size(); ++i) {
    const int wstart = max<int>(0, i - hwsize);
    const int wend = min<int>(v.size() - 1, i + hwsize);
    vector<double> window(v.begin() + wstart, v.begin() + wend);
    vfilt->push_back(Median<double>(&window));
  }
}

void ColorChannelKDE(const uint8_t* data,
                     const uint8_t* mask,
                     int W,
                     int H,
                     bool median_filter,
                     std::vector<double>* target_prob) {
  vector<double> xis;
  for (int i = 0; i < W*H; ++i) {
    if (mask[i]) {
      xis.push_back(data[i]);
    }
  }

  ColorChannelKDE(xis, median_filter, target_prob);
}

void ColorChannelKDE(const uint8_t* data,
                     const vector<Scribble>& scribbles,
                     bool background,
                     int W,
                     int H,
                     bool median_filter,
                     std::vector<double>* target_prob) {
  vector<double> xis;
  for (const Scribble& s : scribbles) {
    if (s.background == background) {
      for (const Point2i& p : s.pixels) {
        xis.push_back(data[W*p.y + p.x]);
      }
    }
  }
  ColorChannelKDE(xis, median_filter, target_prob);
}

void ColorChannelKDE(const std::vector<double>& xis,
                     bool median_filter,
                     std::vector<double>* target_prob) {
  vector<double> weights(xis.size(), 1/(double)xis.size());
  vector<double> targets;
  for (int i = 0; i < 255; ++i) {
    targets.push_back(i);
  }
  //UnivariateKDE(xis, weights, targets, target_prob);
  FastUnivariateKDE(xis, weights, targets, target_prob);

  if (median_filter) {
    vector<double> medfilt;
    MedianFilter(*target_prob, 5, &medfilt);
    *target_prob = medfilt;
  }

}
