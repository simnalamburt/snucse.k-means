#include <vector>
#include <limits>
#include <algorithm>
#include <cstring>
#include "kmeans.h"

using std::vector;
using std::numeric_limits;
using std::fill;

void kmeans(int iteration_n, int class_n, int data_n, Point* centroids, Point* data, int* partitioned) {
  vector<int> count(class_n);

  // Iterate through number of interations
  for (int _ = 0; _ < iteration_n; ++_) {
    // Assignment step
    for (int i = 0; i < data_n; ++i) {
      auto min_dist = numeric_limits<float>::max();
      for (int j = 0; j < class_n; ++j) {
        Point t;
        t.x = data[i].x - centroids[j].x;
        t.y = data[i].y - centroids[j].y;

        float dist = t.x * t.x + t.y * t.y;
        if (dist < min_dist) {
          partitioned[i] = j;
          min_dist = dist;
        }
      }
    }

    // Update step
    // Clear sum buffer and class count
    memset(centroids, 0, class_n * sizeof *centroids);
    fill(count.begin(), count.end(), 0);

    // Sum up and count data for each class
    for (int i = 0; i < data_n; ++i) {
      centroids[partitioned[i]].x += data[i].x;
      centroids[partitioned[i]].y += data[i].y;
      ++count[partitioned[i]];
    }

    // Divide the sum with number of class for mean point
    for (int i = 0; i < class_n; ++i) {
      centroids[i].x /= count[i];
      centroids[i].y /= count[i];
    }
  }
}
