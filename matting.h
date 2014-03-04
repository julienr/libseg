#ifndef _LIBMATTING_MATTING_H_
#define _LIBMATTING_MATTING_H_

#include <cstdint>
#include "utils.h"

// Computes P(c_x | M) where M is background/foreground estimated by KDE from
// the mask. c_x is a pixel.
//
// outimg should be a user-allocated W*H image
void ImageColorPDF(const uint8_t* const* channels,
                   const uint8_t* mask,
                   int W,
                   int H,
                   double* outimg);

void ImageColorPDF(uint8_t const* const* channels,
                   const std::vector<Scribble>& scribbles,
                   bool background,
                   int W,
                   int H,
                   double* outimg);

void ImageColorPDF(uint8_t const* const* channels,
                   const std::vector<std::vector<double>>& probs,
                   int W,
                   int H,
                   double* outimg);


// Equation 1. of Bai09 :
// P_F(cx) = P(cx|F) / (P(cx|F) + P(cx|B))
//
// likelihood should be a user-allocated W*H image
void ForegroundLikelihood(const double* P_cx_F,
                          const double* P_cx_B,
                          int W,
                          int H,
                          double* likelihood);

// Compute final foreground mask
// outmask is allocated by the caller as a W*H row-major array
void FinalForegroundMask(const double* fg_dist,
                         const double* bg_dist,
                         int W,
                         int H,
                         uint8_t* outmask);

#endif
