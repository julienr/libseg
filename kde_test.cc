#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>

#include "kde.h"

using namespace std;
using ::testing::DoubleNear;

namespace {

TEST(FastUnivariateKDE, FastSlow) {
  // Ensure UnivariateKDE and FastUnivariateKDE produce the same results

  // Source
  vector<double> x{0.7165, 0.5113, 0.7764, 0.4893, 0.1859};

  // source weights
  vector<double> w{0.2280, 0.4496, 0.1722, 0.9688, 0.3557};

  // Targets
  vector<double> y{0.9561, 0.5955, 0.0287, 0.8121, 0.6101, 0.7015, 0.0922,
                   0.4249, 0.3756, 0.1662, 0.8332, 0.8386, 0.4516, 0.9566,
                   0.1472, 0.8699, 0.7694, 0.4442, 0.6206};

  const double epsilon = 1e-2;

  vector<double> slow_prob;
  vector<double> fast_prob;
  UnivariateKDE(x, w, y, &slow_prob);
  FastUnivariateKDE(x, w, y, &fast_prob, epsilon);

  ASSERT_EQ(slow_prob.size(), fast_prob.size());
  for (size_t i = 0; i < slow_prob.size(); ++i) {
    ASSERT_THAT(slow_prob[i], DoubleNear(fast_prob[i], 1e-2))
      << "At index " << i << " : " << slow_prob[i] << " != " << fast_prob[i];
  }
}

}
