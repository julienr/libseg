#ifndef _LIBMATTING_KDE_H_
#define _LIBMATTING_KDE_H_

// Implement Kernel Density Estimation (KDE) to estimate the color PDF
//   Pr(color | foreground)
// This is described in section 3.1.1 (and fig.3) of Bai09. Note that like
// in Bai, we perform one univariate KDE on each axis of the colorspace instead
// of a multivariate 3D KDE
#include <cstdint>
#include <vector>

#include "utils.h"

// Use kernel density estimation to compute the probability of each point
// int targets (storing the probability in target_prob) given the data points
// in xis and their corresponding weights
//
// p = \sum_i w_i K(t - xi)
//
// Where K is a gaussian kernel and t is the target. w_i is the weight
//
// The KDE bandwidth is estimated automatically using a rule-of-thumb
void UnivariateKDE(const std::vector<double>& xis,
                   const std::vector<double>& weights,
                   const std::vector<double>& targets,
                   std::vector<double>* target_prob);

// All matrices are contiguous arrays containing entries in row major format
// - dim : data dimensionality
// - N : number of source points
// - M : number of target points
// - xis : N x d matrix of N source points in d dimensions
// - weights : array of N source weights
// - targets : M x d matrix of N target poitns in d dimensions
//
// - target_prob : array of size M. Output probability for each target.
//                 (assumed to be allocated by the caller)
void FastKDE(int dim,
             int N,
             int M,
             const double* xis,
             const double* weights,
             const double* targets,
             double* target_prob,
             double epsilon=1e-2);

// Uses the figtree library to compute fast KDE with the help of the
// Gauss transform
// epsilon is the desired maximum absolute error after normalizing output
// by sum of weights.
// If the weights, q_i (see below), add up to 1, then this is will be the
// maximum absolute error.
void FastUnivariateKDE(const std::vector<double>& xis,
                       const std::vector<double>& weights,
                       const std::vector<double>& targets,
                       std::vector<double>* target_prob,
                       double epsilon=1e-2);

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
                     std::vector<double>* target_prob);

// Same as above, but uses a list of scribbles instead of a mask. Only
// scribbles with s.background == background are considered.
void ColorChannelKDE(const uint8_t* data,
                     const std::vector<Scribble>& scribbles,
                     bool background,
                     int W,
                     int H,
                     bool median_filter,
                     std::vector<double>* target_prob);

void ColorChannelKDE(const std::vector<double>& xi,
                     bool median_filter,
                     std::vector<double>* target_prob);

#endif
