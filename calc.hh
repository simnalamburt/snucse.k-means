#include <vector>
#include <limits>
#include <cstring>

namespace {
  using namespace std;

  struct point { float x, y; };

  void kmeans(
      const int repeat,
      const int class_n, const int data_n,
      point *const centroids, point *const data, int *const table)
  {
    for (int _ = 0; _ < repeat; ++_) {
      // Assignment step
      #pragma omp parallel for
      for (int i = 0; i < data_n; ++i) {
        auto min_dist = numeric_limits<float>::max();
        for (int j = 0; j < class_n; ++j) {
          const float x = data[i].x - centroids[j].x;
          const float y = data[i].y - centroids[j].y;
          const float dist = x*x + y*y;
          if (dist < min_dist) {
            table[i] = j;
            min_dist = dist;
          }
        }
      }

      // Update step
      memset(centroids, 0, class_n * sizeof *centroids);
      vector<int> count(class_n);

      // Calculate mean value
      for (int i = 0; i < data_n; ++i) {
        centroids[table[i]].x += data[i].x;
        centroids[table[i]].y += data[i].y;
        ++count[table[i]];
      }

      for (int i = 0; i < class_n; ++i) {
        centroids[i].x /= count[i];
        centroids[i].y /= count[i];
      }
    }
  }
}
