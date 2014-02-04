#ifndef _LIBMATTING_KDE_H_
#define _LIBMATTING_KDE_H_

// Implement Kernel Density Estimation (KDE) to estimate the color PDF
//   Pr(color | foreground)
// This is described in section 3.1.1 (and fig.3) of Bai09. Note that like
// in Bai, we perform one univariate KDE on each axis of the colorspace instead
// of a multivariate 3D KDE
#include <cstdint>
#include <vector>

// Use kernel density estimation to compute the probability of each point
// int targets (storing the probability in target_prob) given the data points
// in xis and their corresponding weights
//
// p = \sum_i w_i K(t - xi)
//
// Where K is a gaussian kernel and t is the target. w_i is the weight
//
// The KDE bandwidth is estimated automatically using a rule-of-thumb
void UnivariateKDE(const std::vector<float>& xis,
                   const std::vector<float>& weights,
                   const std::vector<float>& targets,
                   std::vector<float>* target_prob);

// Helper function to compute KDE on a single color channel for values of
// x in the [0, 255] interval.
// So, target_prob will have 256 entries containing the probability for each
// 8bit color value.
// (Optionally) A median filter is also applied to smooth the probabilities
void ColorChannelKDE(const uint8_t* data,
                     const uint8_t* mask,
                     int W,
                     int H,
                     bool median_filter,
                     std::vector<float>* target_prob);

#endif
