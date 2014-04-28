#ifndef _LIBMATTING_API_H_
#define _LIBMATTING_API_H_

#include <memory>
#include <cstdint>
#include <string.h>

#include "utils.h"

class Matter {
 public:
  Matter(uint8_t* lab_l, uint8_t* lab_a,
         uint8_t* lab_b, int W, int H);
  virtual ~Matter();

  // Fill mask with the foreground mask resulting from the matting.
  // 255 indicates foreground pixels, 0 background.
  void GetForegroundMask(uint8_t* mask);

  void GetForegroundLikelihood(double* out);
  void GetBackgroundLikelihood(double* out);

  void GetForegroundDist(double* out);
  void GetBackgroundDist(double* out);

 protected:
  int W, H;
  std::unique_ptr<uint8_t[]> lab_l, lab_a, lab_b;
  // TODO: We do not actually need the pdf for each pixel of the image. Use
  // a simple lookup table of pixel intensity to pdf instead
  std::unique_ptr<double[]> fg_pdf, bg_pdf;
  std::unique_ptr<double[]> fg_likelihood, bg_likelihood;
  std::unique_ptr<double[]> fg_dist, bg_dist;
  std::unique_ptr<uint8_t[]> final_mask;

  uint8_t* channels[3];
};

// A simpler API that doesn't have the notion of scribbles ordering, but just
// use one mask for background scribbles and one mask for foreground
class SimpleMatter : public Matter {
 public:
  // The image is given in the lab colorspace. Each of lab_l, lab_a, lab_b is
  // a W*H array stored in row-major order
  // Matter makes an internal copy of the image
  SimpleMatter(uint8_t* lab_l, uint8_t* lab_a, uint8_t* lab_b, int W, int H);
  virtual ~SimpleMatter();

  void SetScribblesMasks(uint8_t* bg_mask, uint8_t* fg_mask);
};

// Contains the current state of the matting
// This is an API that implements section 3.1.3 of [Bai09] regarding user
// interaction. Basically, scribbles ordering matters and a new background
// scribble will only interfer with what's current foreground and inversely.
class InteractiveMatter : public Matter {
 public:
  // The image is given in the lab colorspace. Each of lab_l, lab_a, lab_b is
  // a W*H array stored in row-major order
  // Matter makes an internal copy of the image
  InteractiveMatter(uint8_t* lab_l, uint8_t* lab_a,
                    uint8_t* lab_b, int W, int H);
  virtual ~InteractiveMatter();


  // Add a scribble. Note that empty scribbles (0 pixels) will be ignored
  //
  // We do not store all scribbles as a simple boolean mask because
  // of the way user interaction is handled (see Bai09). Scribbles ordering is
  // important to let user do local corrections that do not influence
  // the whole image. In other words, doing the matting with the same set of
  // scribbles but in different order might result in different result.
  void AddScribble(const Scribble& s);

  int NumScribbles() {
    return scribbles.size();
  }

 private:
  // Initially false, true when at least one scribble has been added to bg/fg
  bool bg_scribbled_, fg_scribbled_;

  std::vector<Scribble> scribbles;
};

#endif
