#include "api.h"

#include "kde.h"
#include "geodesic.h"
#include "matting.h"

#include <glog/logging.h>
#include <limits>

using namespace std;
using boost::scoped_array;

Matter::Matter(uint8_t* l, uint8_t* a, uint8_t* b,
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
    final_mask(new uint8_t[W*H]),
    bg_scribbled_(false),
    fg_scribbled_(false) {
  memcpy(lab_l.get(), l, sizeof(uint8_t)*W*H);
  memcpy(lab_a.get(), a, sizeof(uint8_t)*W*H);
  memcpy(lab_b.get(), b, sizeof(uint8_t)*W*H);

  for (int i = 0; i < W*H; ++i) {
    final_mask[i] = 0;
    fg_dist[i] = numeric_limits<double>::max();
    bg_dist[i] = numeric_limits<double>::max();
  }

  channels[0] = lab_l.get();
  channels[1] = lab_a.get();
  channels[2] = lab_b.get();
}


Matter::~Matter() {}

void Matter::AddScribble(const Scribble& s) {
  if (s.pixels.size() == 0) {
    LOG(WARNING) << "Ignoring empty scribble";
    return;
  }
  scribbles.push_back(s);

  // 1. Update bg or fg pdf (depending on scribble's background attribute)
  if (s.background) {
    //cout << "Updating bg pdf" << endl;
    ImageColorPDF(channels, scribbles, true, W, H, bg_pdf.get());
  } else {
    //cout << "Updating fg pdf" << endl;
    ImageColorPDF(channels, scribbles, false, W, H, fg_pdf.get());
  }

  // 2. Update fg AND bg likelihood
  ForegroundLikelihood(fg_pdf.get(), bg_pdf.get(), H, W, fg_likelihood.get());
  ForegroundLikelihood(bg_pdf.get(), fg_pdf.get(), H, W, bg_likelihood.get());

#if 0
  // 4. Update fg or bg distance map, but only for pixels within the
  //    fg (if bg scribble) or bg (if fg scribble).
  scoped_array<double> newdist(new double[W*H]);
  if (s.background) {
    GeodesicDistanceMap(scribbles, true, bg_likelihood.get(), W, H,
                        newdist.get());
    if (!bg_scribbled_) { // special case for first scribble
      memcpy(bg_dist.get(), newdist.get(), sizeof(double)*W*H);
      bg_scribbled_ = true;
    } else {
      MaskedCopy<double, uint8_t>(newdist.get(), final_mask.get(), 255,
                                  W*H, bg_dist.get());
    }
  } else {
    GeodesicDistanceMap(scribbles, false, fg_likelihood.get(), W, H,
                        newdist.get());
    if (!fg_scribbled_) { // special case for first scribble
      memcpy(fg_dist.get(), newdist.get(), sizeof(double)*W*H);
      fg_scribbled_ = true;
    } else {
      MaskedCopy<double, uint8_t>(newdist.get(), final_mask.get(), 0,
                                  W*H, fg_dist.get());

    }
  }
#else
  // 4. (alternative) Update everything
  GeodesicDistanceMap(scribbles, true, bg_likelihood.get(), W, H,
                      bg_dist.get());
  GeodesicDistanceMap(scribbles, false, fg_likelihood.get(), W, H,
                      fg_dist.get());
#endif

  // 5. Compute final mask
  FinalForegroundMask(fg_dist.get(), bg_dist.get(), W, H, final_mask.get());
}

void Matter::GetForegroundLikelihood(double* out) {
  memcpy(out, fg_likelihood.get(), sizeof(double)*W*H);
}

void Matter::GetBackgroundLikelihood(double* out) {
  memcpy(out, bg_likelihood.get(), sizeof(double)*W*H);
}

void Matter::GetForegroundDist(double* out) {
  memcpy(out, fg_dist.get(), sizeof(double)*W*H);
}

void Matter::GetBackgroundDist(double* out) {
  memcpy(out, bg_dist.get(), sizeof(double)*W*H);
}

void Matter::GetForegroundMask(uint8_t* outmask) {
  memcpy(outmask, final_mask.get(), sizeof(uint8_t)*W*H);

}
