#include "api.h"

#include "kde.h"
#include "geodesic.h"
#include "matting.h"

using namespace std;
using boost::scoped_array;

MattingState::MattingState(uint8_t* l, uint8_t* a, uint8_t* b,
                           int W, int H)
  : W(W), H(H),
    lab_l(new uint8_t[W*H]),
    lab_a(new uint8_t[W*H]),
    lab_b(new uint8_t[W*H]),
    fg_pdf(new double[W*H]),
    bg_pdf(new double[W*H]),
    fg_likelihood(new double[W*H]),
    bg_likelihood(new double[W*H]),
    fg_dist(new double[W*H]),
    bg_dist(new double[W*H]),
    final_mask(new uint8_t[W*H]) {
  memcpy(lab_l.get(), l, sizeof(uint8_t)*W*H);
  memcpy(lab_a.get(), a, sizeof(uint8_t)*W*H);
  memcpy(lab_b.get(), b, sizeof(uint8_t)*W*H);

  channels[0] = lab_l.get();
  channels[1] = lab_a.get();
  channels[2] = lab_b.get();
}


MattingState::~MattingState() {}

void MattingState::AddScribble(const Scribble& s) {
  scribbles.push_back(s);

  // 1. Update bg or fg pdf (depending on scribble's background attribute)
  if (s.background) {
    ImageColorPDF(channels, scribbles, true, W, H, bg_pdf.get());
  } else {
    ImageColorPDF(channels, scribbles, false, W, H, fg_pdf.get());
  }

  // 2. Update fg AND bg likelihood
  ForegroundLikelihood(fg_pdf.get(), bg_pdf.get(), H, W, fg_likelihood.get());
  ForegroundLikelihood(bg_pdf.get(), fg_pdf.get(), H, W, bg_likelihood.get());

  // 4. Update fg or bg distance map, but only for pixels within the
  //    fg (if bg scribble) or bg (if fg scribble).
  scoped_array<double> newdist(new double[W*H]);
  if (s.background) {
    GeodesicDistanceMap(scribbles, true, bg_likelihood.get(), W, H,
                        newdist.get());
    MaskedCopy<double, uint8_t>(newdist.get(), final_mask.get(), 255,
                                W*H, bg_dist.get());
  } else {
    GeodesicDistanceMap(scribbles, false, fg_likelihood.get(), W, H,
                        newdist.get());
    MaskedCopy<double, uint8_t>(newdist.get(), final_mask.get(), 0,
                                W*H, fg_dist.get());
  }

  // 5. Compute final mask
  FinalForegroundMask(fg_dist.get(), bg_dist.get(), W, H, final_mask.get());
}
