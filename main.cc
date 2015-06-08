#include <vector>
#include <memory>
#include <limits>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <ctime>
#include "mpi.h"

#define DATA_DIM 2
#define DEFAULT_ITERATION 1024

using namespace std;


namespace {
  struct point { float x, y; };

  unique_ptr<float[]> init_data(char *name, uint32_t &size);
  int timespec_subtract(struct timespec*, struct timespec*, struct timespec*);
  void kmeans(int repeat, int class_n, int data_n, point *centroids, point *data, int *table);

  // Dead-Simple MPI wrapper
  class mpi {
    int _rank, _size;
  public:
    mpi(int &argc, char **&argv) {
      MPI_Init(&argc, &argv);
      MPI_Comm_size(MPI_COMM_WORLD, &_size);
      MPI_Comm_rank(MPI_COMM_WORLD, &_rank);
    }

    ~mpi() {
      MPI_Finalize();
    }

    int rank() const { return _rank; }
    int size() const { return _size; }

    bool root() const { return rank() == 0; }
  };
}


//
// Entry point
//
int main(int argc, char** argv) {
  const mpi mpi(argc, argv);

  // Check parameters
  if (argc < 4) {
    if (mpi.root()) {
      fprintf(stderr, "usage: %s <centroid file> <data file> <paritioned result> [<final centroids>] [<iteration number>]\n", argv[0]);
    }
    return 1;
  }

  // Read initial centroid data and input data
  size_t iteration_n = argc > 5 ? atoi(argv[5]) : DEFAULT_ITERATION;

  uint32_t class_n, data_n;
  auto centroids = init_data(argv[1], class_n);
  auto data = init_data(argv[2], data_n);
  auto partitioned = unique_ptr<int[]>(new int[data_n]);

  // Start timer
  struct timespec start;
  MPI_Barrier(MPI_COMM_WORLD);
  if (mpi.root()) { clock_gettime(CLOCK_MONOTONIC, &start); }

  // Run Kmeans algorithm
  kmeans(iteration_n, class_n, data_n, (point*)&centroids[0], (point*)&data[0], &partitioned[0]);

  // Stop timer
  if (mpi.root()) {
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);

    struct timespec spent;
    timespec_subtract(&spent, &end, &start);
    printf("Time spent: %ld.%09ld\n", spent.tv_sec, spent.tv_nsec);

    // Write classified result
    FILE *output = fopen(argv[3], "wb");
    fwrite(&data_n, sizeof(data_n), 1, output);
    fwrite(&partitioned[0], sizeof(int), data_n, output);
    fclose(output);

    // Write final centroid data
    if (argc > 4) {
      FILE *output = fopen(argv[4], "wb");
      fwrite(&class_n, sizeof(class_n), 1, output);
      fwrite(&centroids[0], sizeof(point), class_n, output);
      fclose(output);
    }
  }

  return 0;
}


namespace {
  unique_ptr<float[]> init_data(char *name, uint32_t &size) {
    FILE *input = fopen(name, "rb");
    if (input == NULL) {
      fprintf(stderr, "File open error %s\n", name);
      exit(EXIT_FAILURE);
    }

    if (fread(&size, sizeof(uint32_t), 1, input) < 1) {
      fputs("Error reading file size", stderr);
      exit(EXIT_FAILURE);
    }

    auto mem = unique_ptr<float[]>(new float[size*DATA_DIM]);
    if (fread(&mem[0], sizeof(float), DATA_DIM*size, input) < DATA_DIM*size) {
      fputs("Error reading data", stderr);
      exit(EXIT_FAILURE);
    }

    fclose(input);
    return mem;
  }

  int timespec_subtract (struct timespec* result, struct timespec *x, struct timespec *y) {
    /* Perform the carry for the later subtraction by updating y. */
    if (x->tv_nsec < y->tv_nsec) {
      int nsec = (y->tv_nsec - x->tv_nsec) / 1000000000 + 1;
      y->tv_nsec -= 1000000000 * nsec;
      y->tv_sec += nsec;
    }
    if (x->tv_nsec - y->tv_nsec > 1000000000) {
      int nsec = (x->tv_nsec - y->tv_nsec) / 1000000000;
      y->tv_nsec += 1000000000 * nsec;
      y->tv_sec -= nsec;
    }

    /* Compute the time remaining to wait.
       tv_nsec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_nsec = x->tv_nsec - y->tv_nsec;

    /* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
  }


  void kmeans(
      const int repeat,
      const int class_n, const int data_n,
      point *const centroids, point *const data, int *const table)
  {
    for (int _ = 0; _ < repeat; ++_) {
      int size, rank;
      MPI_Comm_size(MPI_COMM_WORLD, &size);
      MPI_Comm_rank(MPI_COMM_WORLD, &rank);
      const int begin = data_n*rank/size;
      const int end = data_n*(rank + 1)/size;

      // Assignment step
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

      const int count = end - begin;
      MPI_Gather(&table[begin], count, MPI_INT, &table[0], count, MPI_INT, 0, MPI_COMM_WORLD);

      // Update step
      if (rank == 0) {
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

      MPI_Bcast(&centroids[0], sizeof(point) * class_n, MPI_BYTE, 0, MPI_COMM_WORLD);
    }
  }
}
