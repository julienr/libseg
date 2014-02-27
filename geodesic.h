#ifndef _GEODESIC_H_
#define _GEODESIC_H_

#include <cstdint>

// Functions to compute geodesic distance between image pixels and user
// scribbles as per section 3.1.2 (fig.5) of Bai09

// For a W*H image (4-connected graph) given as a heightmap, compute, for each
// pixel, the minimum geodesic distance to the closest source (source_mask
// = true)
// This is described in section 3.1.2 (fig. 5) of Bai09
void GeodesicDistanceMap(const uint8_t* source_mask,
                         const double* height,
                         int W, int H,
                         double* dists);

#endif
