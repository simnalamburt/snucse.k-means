#include <vector>
#include <limits>
#include <algorithm>
#include <cstring>
#include <pthread.h>
#include "kmeans.h"

using std::vector;
using std::pair;
using std::numeric_limits;
using std::for_each;
using std::fill;

typedef Point point_t;

struct shared_context {
  int class_n, data_n, *partitioned;
  point_t *centroids, *data;
};

struct context {
  shared_context *shared_ctxt;
  int begin, end;
};

void *per_thread(void *data) {
  context &ctxt = *(context*)data;
  shared_context &s = *(shared_context*)ctxt.shared_ctxt;

  for (int i = ctxt.begin; i < ctxt.end; ++i) {
    auto min_dist = numeric_limits<float>::max();
    for (int j = 0; j < s.class_n; ++j) {
      const float x = s.data[i].x - s.centroids[j].x;
      const float y = s.data[i].y - s.centroids[j].y;
      const float dist = x*x + y*y;
      if (dist < min_dist) {
        s.partitioned[i] = j;
        min_dist = dist;
      }
    }
  }

  return 0;
}

void kmeans(const int iteration_n, const int class_n, const int data_n,
    point_t *const centroids, point_t *const data, int *const partitioned)
{
  const int thread_count = 4;

  shared_context shared_ctxt;
  shared_ctxt.class_n = class_n;
  shared_ctxt.data_n = data_n;
  shared_ctxt.centroids = centroids;
  shared_ctxt.data = data;
  shared_ctxt.partitioned = partitioned;

  // 과제 스펙때문에 std::thread 대신 pthread를 사용
  vector<pair<pthread_t, context>> pthreads;
  for (int t = 0; t < thread_count; ++t) {
    context ctxt;
    ctxt.shared_ctxt = &shared_ctxt;
    ctxt.begin = data_n*t/thread_count;
    ctxt.end = data_n*(t+1)/thread_count;

    pthreads.emplace_back(pthread_t(), ctxt);
  }

  for (int _ = 0; _ < iteration_n; ++_) {
    // Create threads
    const auto end = pthreads.cend();
    for (auto it = pthreads.begin(); it != end; ++it) {
      pthread_create(&it->first, 0, per_thread, &it->second);
    }

    // Wait for all threads
    for (auto it = pthreads.begin(); it != end; ++it) {
      pthread_join(it->first, 0);
    }

    // Update step
    memset(centroids, 0, class_n * sizeof *centroids);
    vector<int> count(class_n);

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
