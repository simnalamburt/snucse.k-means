#include <iostream>
#include <memory>
#include <array>
#include <vector>
#include <thread>
#include <cstring>
#include <CL/cl.h>

namespace {


using namespace std;

struct point { float x, y; };

void check(cl_int);

void kmeans(
    const int repeat,
    const int class_n, const int data_n,
    point *const centroids, point *const data, int *const table)
{
  const size_t size_centroids = class_n * sizeof *centroids;
  const size_t size_data = data_n * sizeof *data;
  const size_t size_table = data_n * sizeof *table;

  //
  // Initialize OpenCL resources
  //
  cl_platform_id platform;
  check(clGetPlatformIDs(1, &platform, NULL));

  cl_device_id device;
  check(clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL));

  cl_int e;
  auto ctxt = clCreateContext(NULL, 1, &device, NULL, NULL, &e); check(e);
  auto cmdq = clCreateCommandQueue(ctxt, device, 0, &e); check(e);

  auto buffer_centroids = clCreateBuffer(ctxt, CL_MEM_READ_ONLY, size_centroids, NULL, &e); check(e);
  auto buffer_data = clCreateBuffer(ctxt, CL_MEM_READ_ONLY, size_data, NULL, &e); check(e);
  auto buffer_table = clCreateBuffer(ctxt, CL_MEM_WRITE_ONLY, size_table, NULL, &e); check(e);


  //
  // Compile OpenCL kernel codes
  //
  auto code = R"(
    typedef struct { float x, y; } point;

    __kernel void calc(
          int class_n,
          __global point *centroids,
          __global point *data,
          __global int *table) {
      int i = get_global_id(0);
      float min_dist = 3.402823e+38; // Numeric limits
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
  )";
  const auto codelen = strlen(code);
  auto program = clCreateProgramWithSource(ctxt, 1, &code, &codelen, &e); check(e);
  e = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
  if (e == CL_BUILD_PROGRAM_FAILURE) {
    // Print detailed message
    cerr << endl;
    cerr << "OpenCL compile error" << endl;

    size_t len, size = 2048;
    auto buffer = unique_ptr<char[]>(new char[2048]);
    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, size, buffer.get(), &len);
    cerr << buffer.get() << endl;
    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_STATUS, size, buffer.get(), &len);
    cerr << buffer.get() << endl;
    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_OPTIONS, size, buffer.get(), &len);
    cerr << buffer.get() << endl;
    exit(1);
  }
  check(e);


  //
  // Create kernel
  //
  auto kernel = clCreateKernel(program, "calc", &e); check(e);
  check(clSetKernelArg(kernel, 0, sizeof(cl_int), &class_n));
  check(clSetKernelArg(kernel, 1, sizeof(cl_mem), &buffer_centroids));
  check(clSetKernelArg(kernel, 2, sizeof(cl_mem), &buffer_data));
  check(clSetKernelArg(kernel, 3, sizeof(cl_mem), &buffer_table));


  for (int _ = 0; _ < repeat; ++_) {
    //
    // Calculate
    //
    check(clEnqueueWriteBuffer(cmdq, buffer_centroids, CL_FALSE, 0, size_centroids, centroids, 0, NULL, NULL));
    check(clEnqueueWriteBuffer(cmdq, buffer_data, CL_FALSE, 0, size_data, data, 0, NULL, NULL));

    array<size_t, 1> global = {{ data_n }};
    array<size_t, 1> local = {{ 256 }};
    check(clEnqueueNDRangeKernel(cmdq, kernel, 1, NULL, global.data(), local.data(), 0, NULL, NULL));
    check(clEnqueueReadBuffer(cmdq, buffer_table, CL_TRUE, 0, size_table, table, 0, NULL, NULL));

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


  //
  // Release OpenCL resources
  //
  check(clReleaseMemObject(buffer_centroids));
  check(clReleaseMemObject(buffer_data));
  check(clReleaseMemObject(buffer_table));
  check(clReleaseContext(ctxt));
  check(clReleaseKernel(kernel));
  check(clReleaseProgram(program));
  check(clReleaseCommandQueue(cmdq));
}

const char *clGetErrorMessage(cl_int error_code) {
  switch (error_code) {
  // run-time and JIT compiler errors
  case 0: return "CL_SUCCESS";
  case -1: return "CL_DEVICE_NOT_FOUND";
  case -2: return "CL_DEVICE_NOT_AVAILABLE";
  case -3: return "CL_COMPILER_NOT_AVAILABLE";
  case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
  case -5: return "CL_OUT_OF_RESOURCES";
  case -6: return "CL_OUT_OF_HOST_MEMORY";
  case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
  case -8: return "CL_MEM_COPY_OVERLAP";
  case -9: return "CL_IMAGE_FORMAT_MISMATCH";
  case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
  case -11: return "CL_BUILD_PROGRAM_FAILURE";
  case -12: return "CL_MAP_FAILURE";
  case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
  case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
  case -15: return "CL_COMPILE_PROGRAM_FAILURE";
  case -16: return "CL_LINKER_NOT_AVAILABLE";
  case -17: return "CL_LINK_PROGRAM_FAILURE";
  case -18: return "CL_DEVICE_PARTITION_FAILED";
  case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

  // compile-time errors
  case -30: return "CL_INVALID_VALUE";
  case -31: return "CL_INVALID_DEVICE_TYPE";
  case -32: return "CL_INVALID_PLATFORM";
  case -33: return "CL_INVALID_DEVICE";
  case -34: return "CL_INVALID_CONTEXT";
  case -35: return "CL_INVALID_QUEUE_PROPERTIES";
  case -36: return "CL_INVALID_COMMAND_QUEUE";
  case -37: return "CL_INVALID_HOST_PTR";
  case -38: return "CL_INVALID_MEM_OBJECT";
  case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
  case -40: return "CL_INVALID_IMAGE_SIZE";
  case -41: return "CL_INVALID_SAMPLER";
  case -42: return "CL_INVALID_BINARY";
  case -43: return "CL_INVALID_BUILD_OPTIONS";
  case -44: return "CL_INVALID_PROGRAM";
  case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
  case -46: return "CL_INVALID_KERNEL_NAME";
  case -47: return "CL_INVALID_KERNEL_DEFINITION";
  case -48: return "CL_INVALID_KERNEL";
  case -49: return "CL_INVALID_ARG_INDEX";
  case -50: return "CL_INVALID_ARG_VALUE";
  case -51: return "CL_INVALID_ARG_SIZE";
  case -52: return "CL_INVALID_KERNEL_ARGS";
  case -53: return "CL_INVALID_WORK_DIMENSION";
  case -54: return "CL_INVALID_WORK_GROUP_SIZE";
  case -55: return "CL_INVALID_WORK_ITEM_SIZE";
  case -56: return "CL_INVALID_GLOBAL_OFFSET";
  case -57: return "CL_INVALID_EVENT_WAIT_LIST";
  case -58: return "CL_INVALID_EVENT";
  case -59: return "CL_INVALID_OPERATION";
  case -60: return "CL_INVALID_GL_OBJECT";
  case -61: return "CL_INVALID_BUFFER_SIZE";
  case -62: return "CL_INVALID_MIP_LEVEL";
  case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
  case -64: return "CL_INVALID_PROPERTY";
  case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
  case -66: return "CL_INVALID_COMPILER_OPTIONS";
  case -67: return "CL_INVALID_LINKER_OPTIONS";
  case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";

  // extension errors
  case -1000: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
  case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
  case -1002: return "CL_INVALID_D3D10_DEVICE_KHR";
  case -1003: return "CL_INVALID_D3D10_RESOURCE_KHR";
  case -1004: return "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR";
  case -1005: return "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR";
  default: return "Unknown OpenCL error";
  }
}

void check(cl_int err) {
  if (err == CL_SUCCESS) { return; }
  cerr << clGetErrorMessage(err) << endl;
  exit(1);
}


}
