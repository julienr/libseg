#include "geodesic.h"

#include <queue>
#include <algorithm>
#include <limits>
#include <iostream>
#include <string.h>

using namespace std;

void GeodesicDistanceMap(const uint8_t* source_mask,
                         const double* height,
                         int W, int H,
                         double* dists) {
  // The algorithm is actually equivalent to running Dijkstra once for each
  // source and then keeping the minimum distance.
  // This is similar to "SHORTEST-PATH FOREST WITH TOPOLOGICAL ORDERING"
  //
  // But we do it all at once so it should be faster. It works as follow :
  //   1. assign a distance of 0 to all source nodes, infinity to others
  //   2. create a list of unvisited nodes consisting of the source nodes
  //   3. visit neighbors of the current node
  //      - if the current node allows a shortest path to the neighbor
  //        - update the neighbor dist
  //        - add the neighbor to the unvisited nodes
  //   4. remove current node from visited
  //   5. pick the node with the smallest distance from the unvisited node as
  //      the new current
  const int N = W*H;
  // priority queue
  typedef pair<int, double> PriorityEntry;
  auto comp = [](const PriorityEntry& e1, const PriorityEntry& e2) {
    return e1.second > e2.second;
  };
  priority_queue<PriorityEntry, vector<PriorityEntry>, decltype(comp)> Q(comp);

  for (int i = 0; i < N; ++i) {
    if (source_mask[i]) {
      dists[i] = 0;
      //const int x = i % W;
      //const int y = i / W;
      //cout << "source at : " << x << ", " << y << endl;
      Q.push(make_pair(i, 0));
    } else {
      dists[i] = numeric_limits<double>::max();
    }
  }

  //dx dy pairs for neighborhood exploration
  const int dx[4] = {-1, 0, 1,  0};
  const int dy[4] = { 0, 1, 0, -1};

  // main loop
  while(!Q.empty()) {
    int u = Q.top().first;
    const int ux = u % W;
    const int uy = u / W;
    //cout << "u = " << u << " ux : " << ux << ", uy : " << uy << endl;
    Q.pop();
    // explore neighbors
    for (int i = 0; i < 4; ++i) {
      const int vx = ux + dx[i];
      const int vy = uy + dy[i];
      if ((vx < 0 || vx >= W) || (vy < 0 || vy >= H)) {
        continue;
      }
      const int v = vy*W + vx;
      //cout << "v = " << v << " : " << vx << ", " << vy << endl;
      const double w = fabs(height[v] - height[u]);
      //cout << "dist u : " << dists[u] << endl;
      //cout << "dist v : " << dists[v] << endl;
      //cout << "w : " << w << endl;
      if (dists[u] + w < dists[v]) { // we found a shortest path to v
        //cout << "!!! shortest" << endl;
        dists[v] = dists[u] + w;
        Q.push(make_pair(v, dists[v]));
      }
    }
  }
}

