#include "kde.h"
#include <cmath>
#include <algorithm>

#include <glog/logging.h>

using namespace std;

const float NORMAL_FACTOR = 1/(float)sqrt(2*M_PI);

float GaussianKernel(float t, float xi, float h) {
  const float x = (t - xi) / h;
  // TODO: We have to remove NORMAL_FACTOR so that is sums to 1
  // (see plot_densities.py). Is that normal ?

  //return NORMAL_FACTOR * exp(-0.5 * x * x);
  return exp(-0.5 * x * x);
}

// Use Scott's Rule, like scipy :
//   h = n**(-1./(d+4))
// where n is the number of data points, d the number of dimensions
// http://docs.scipy.org/doc/scipy/reference/generated/scipy.stats.gaussian_kde.html
float EstimateBandwidth(int ndata, int ndims) {
  return pow(ndata, -1/(float)(ndims + 4));
}

void UnivariateKDE(const vector<float>& xis,
                   const vector<float>& weights,
                   const vector<float>& targets,
                   vector<float>* target_prob) {
  const float h = EstimateBandwidth(xis.size(), 1);
  LOG(INFO) << "h : " << h;
  for (size_t ti = 0; ti < targets.size(); ++ti) {
    float prob = 0;
    for (size_t i = 0; i < xis.size(); ++i) {
      prob += weights[i] * GaussianKernel(targets[ti], xis[i], h);
    }
    target_prob->push_back(prob);
  }
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

void MedianFilter(const vector<float>& v,
                  size_t hwsize, // half window size
                  vector<float>* vfilt) {
  for (size_t i = 0; i < v.size(); ++i) {
    const size_t wstart = max<size_t>(0, i - hwsize);
    const size_t wend = min<size_t>(v.size() - 1, i + hwsize);
    vector<float> window(v.begin() + wstart, v.begin() + wend);
    vfilt->push_back(Median<float>(&window));
  }
}

void ColorChannelKDE(const uint8_t* data,
                     const uint8_t* mask,
                     int W,
                     int H,
                     bool median_filter,
                     std::vector<float>* target_prob) {
  vector<float> xis;
  for (int i = 0; i < W*H; ++i) {
    if (mask[i]) {
      xis.push_back(data[i]);
    }
  }
  LOG(INFO) << "xis size : " << xis.size();
  vector<float> weights(xis.size(), 1/(float)xis.size());
  vector<float> targets;
  for (int i = 0; i < 255; ++i) {
    targets.push_back(i);
  }
  UnivariateKDE(xis, weights, targets, target_prob);

  if (median_filter) {
    vector<float> medfilt;
    MedianFilter(*target_prob, 5, &medfilt);
    *target_prob = medfilt;
  }
}
