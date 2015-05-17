#include <vector>
#include <limits>
#include <thread>
#include <cstring>

using std::vector;
using std::thread;
using std::numeric_limits;

struct point_t { float x, y; };
typedef point_t Point;

class context {
  int class_n, data_n;
  point_t *centroids, *data;
  int *table;

public:
  context(int class_n, int data_n, point_t *centroids, point_t *data, int *table) :
    class_n(class_n), data_n(data_n), centroids(centroids), data(data), table(table) { }

  void per_thread(int begin, int end) {
    for (int i = begin; i < end; ++i) {
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
  }
};

void kmeans(const int repeat, const int class_n, const int data_n,
    point_t *const centroids, point_t *const data, int *const table)
{
  const int thread_count = 4;
  context ctxt(class_n, data_n, centroids, data, table);

  for (int _ = 0; _ < repeat; ++_) {
    // Create threads
    vector<thread> threads;
    threads.reserve(thread_count);
    for (int t = 0; t < thread_count; ++t) {
      threads.emplace_back(&context::per_thread, ctxt, data_n*t/thread_count, data_n*(t+1)/thread_count);
    }

    const auto end = threads.cend();
    // Wait for all threads
    for (auto it = threads.begin(); it != end; ++it) { it->join(); }

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
