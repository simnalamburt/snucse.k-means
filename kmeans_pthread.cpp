#include <vector>
#include <limits>
#include <thread>
#include <algorithm>
#include <cstring>
#include "kmeans.h"

using std::vector;
using std::thread;
using std::numeric_limits;
using std::for_each;
using std::fill;

typedef Point point_t;

class context {
  int class_n, data_n;
  point_t *centroids, *data;
  int *partitioned;

public:
  context(int class_n, int data_n, point_t *centroids, point_t *data, int *partitioned) :
    class_n(class_n), data_n(data_n), centroids(centroids), data(data), partitioned(partitioned) { }

  void per_thread(int begin, int end) {
    for (int i = begin; i < end; ++i) {
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
  }
};

void kmeans(int iteration_n, int class_n, int data_n, point_t *centroids, point_t *data, int *partitioned) {
  vector<int> count(class_n);

  for (int _ = 0; _ < iteration_n; ++_) {
    const int thread_count = 4;

    context ctxt(class_n, data_n, centroids, data, partitioned);
    vector<thread> threads;
    for (int t = 0; t < thread_count; ++t) {
      // Create threads
      threads.emplace_back(&context::per_thread, ctxt, data_n*t/thread_count, data_n*(t+1)/thread_count);
    }

    const auto end = threads.cend();
    // Wait for all threads
    for (auto it = threads.begin(); it != end; ++it) { (*it).join(); }

    // Update step
    memset(centroids, 0, class_n * sizeof *centroids);
    fill(count.begin(), count.end(), 0);

    // Calculate mean value
    for (int i = 0; i < data_n; ++i) {
      centroids[partitioned[i]].x += data[i].x;
      centroids[partitioned[i]].y += data[i].y;
      ++count[partitioned[i]];
    }

    for (int i = 0; i < class_n; ++i) {
      centroids[i].x /= count[i];
      centroids[i].y /= count[i];
    }
  }
}
