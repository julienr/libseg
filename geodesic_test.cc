#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>

#include "geodesic.h"

using namespace std;
using ::testing::DoubleNear;

namespace {

TEST(GeodesicDistanceMap, Simple) {
  double height[] = {
    0, 1, 2, 1,
    0, 2, 1, 2,
    0, 1, 0, 1
  };
  uint8_t sources[] = {
    1, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 1, 0
  };
  const int W = 4;
  const int H = 3;
  double dists[W*H];
  GeodesicDistanceMap(sources, height, W, H, dists);

  uint8_t expected_dists[] = {
    0, 1, 2, 3,
    0, 2, 1, 2,
    0, 1, 0, 1,
  };

  //cout << "dists" << endl;
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 4; ++j) {
      ASSERT_EQ(dists[i*W + j], expected_dists[i*W + j])
        << "difference at (" << i << ", " << j << ") : " << dists[i*W+j]
        << " != " << expected_dists[i*W + j];
      //cout << "\t" << dists[i*4 + j];
    }
    //cout << endl;
  }
}


}
