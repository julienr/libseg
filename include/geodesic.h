#ifndef _GEODESIC_H_
#define _GEODESIC_H_

#include <cstdint>
#include "utils.h"

// Functions to compute geodesic distance between image pixels and user
// scribbles as per section 3.1.2 (fig.5) of Bai09

// For a W*H image (4-connected graph) given as a heightmap, compute, for each
// pixel, the minimum geodesic distance to the closest source
// This is described in section 3.1.2 (fig. 5) of Bai09
void GeodesicDistanceMap(const std::vector<Point2i>& sources,
                         const double* height,
                         int W,
                         int H,
                         double* dists);

void GeodesicDistanceMap(const uint8_t* source_mask,
                         const double* height,
                         int W,
                         int H,
                         double* dists);

void GeodesicDistanceMap(const std::vector<Scribble>& scribbles,
                         bool background,
                         const double* height,
                         int W,
                         int H,
                         double* dists);

// Geodesic distance map with multidimensional heightmap
// heightmap is a W*H*dim matrix given in row-major format
void GeodesicDistanceMap(const uint8_t* source_mask,
                         const double* height,
                         int dim,
                         int W,
                         int H,
                         double* dists);

void GeodesicDistanceMap(const std::vector<Point2i>& sources,
                         const double* height,
                         int dim,
                         int W,
                         int H,
                         double* dists);

#endif
