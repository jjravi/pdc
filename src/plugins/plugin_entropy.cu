#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <cinttypes>

#include <omp.h>

#undef NDEBUG
#include <assert.h>

#include <bitset>
#include <iostream>

#include <unordered_map>
#include <vector>
#include <algorithm>

#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <array>
#include <functional>
#include <iomanip>
#include <iostream>

#include <cuda.h>
#include <thrust/sort.h>
#include <thrust/host_vector.h>
#include <thrust/device_vector.h>
#include <thrust/functional.h>
#include <thrust/execution_policy.h>
#include <thrust/transform_reduce.h>
#include <thrust/iterator/discard_iterator.h>

extern "C"
{
#include "pdc_public.h"
#include "plugin_helper.h"
}

#include <mpi.h>

#define CUDA_RUNTIME_API_CALL(apiFuncCall)                                  \
{                                                                           \
  cudaError_t _status = apiFuncCall;                                        \
  if (_status != cudaSuccess) {                                             \
    fprintf(stderr, "%s:%d: error: rt function %s failed with error %s.\n", \
        __FILE__, __LINE__, #apiFuncCall, cudaGetErrorString(_status));       \
    exit(-1);                                                               \
  }                                                                         \
}


static double gettime_ms()
{
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC_RAW, &t);
  return (t.tv_sec+t.tv_nsec*1e-9)*1000;
}

// NOTE: caller needs to free the buffer
void readFileC(const char *filepath, uint8_t **bytes, size_t &num_bytes)
{
  FILE *pFile = fopen(filepath , "rb");
  assert(pFile != NULL);

  // obtain file size:
  if (num_bytes <= 0)
  {
    fseek (pFile, 0L, SEEK_END);
    num_bytes = ftell(pFile);
    fseek(pFile, 0L, SEEK_SET);
  }
  assert(num_bytes > 0);

  // printf("size: %ld bytes\n", num_bytes);

  // allocate memory to contain the whole file:
  // *bytes = (uint8_t*) malloc (sizeof(char)*num_bytes);
  CUDA_RUNTIME_API_CALL(cudaHostAlloc(bytes, sizeof(char)*num_bytes, cudaHostAllocDefault));
  // *bytes = (uint8_t*) malloc (sizeof(char)*num_bytes);
  // *bytes = (uint8_t *)calloc(num_bytes, sizeof(char));
  // assert(*bytes != NULL);

  // copy the file into the *bytes:
  size_t result = fread (*bytes,1,num_bytes,pFile);
  assert(result == num_bytes);

  fclose (pFile);
}

struct freq_calc
{
  const size_t n;
  freq_calc(size_t n) : n(n) {}

  __host__ __device__ float operator()(const int &x) const

  {
    float dx = (float)x;
    float dn = (float)n;
    float prob = dx / dn;
    return (-(prob * log2(prob)));
  }
};

// calculate shannon's entropy
// - Σ i=1 to n     P(x_i) * log P(x_i)
void shannon_entropy(float *buf, size_t nitems)
{
  double tstart = gettime_ms();
  thrust::device_vector<float> d_vbuf(nitems);

  double t30 = gettime_ms();
  CUDA_RUNTIME_API_CALL(cudaMemcpy(thrust::raw_pointer_cast(d_vbuf.data()), buf, nitems*sizeof(float), cudaMemcpyHostToDevice));
  double t31 = gettime_ms();
  // printf("h2d time: %lf ms\n", t31 - t30);

  double t40 = gettime_ms();
  thrust::sort(thrust::device, d_vbuf.begin(), d_vbuf.end(), thrust::greater<float>());
  double t41 = gettime_ms();
  // printf("device sort time: %lf ms\n", t41 - t40);

  double t50 = gettime_ms();
  thrust::device_vector<int> d_values(nitems, 1);
  auto new_end = thrust::reduce_by_key(thrust::device,
    d_vbuf.begin(), d_vbuf.end(), d_values.begin(),
    thrust::make_discard_iterator(), d_values.begin());
  double t51 = gettime_ms();
  // printf("device reduce_by_key time: %lf ms\n", t51 - t50);

  // prints to stdout
  // thrust::copy(d_values.begin(), d_values.begin()+10, std::ostream_iterator<float>(std::cout, ", "));
  // printf("\n");

  thrust::device_vector<int>::iterator iter1 = d_values.begin();
  int values_size = thrust::distance(iter1, new_end.second);

  double t60 = gettime_ms();
  double entropy = thrust::transform_reduce(thrust::device, d_values.begin(), new_end.second, freq_calc(nitems), 0.0f, thrust::plus<float>());
  double t61 = gettime_ms();
  // printf("transform_reduce time: %lf ms\n", t61 - t60);

  double tend = gettime_ms();
  // printf("total time: %lf ms\n", tend - tstart);
  
  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  // printf("entropy: %f\n", entropy);
  printf("rank=%d, entropy=%f\n", rank, entropy);
}

void shannon_entropy_d(double *buf, size_t nitems)
{
  double tstart = gettime_ms();
  thrust::device_vector<double> d_vbuf(nitems);

  double t30 = gettime_ms();
  CUDA_RUNTIME_API_CALL(cudaMemcpy(thrust::raw_pointer_cast(d_vbuf.data()), buf, nitems*sizeof(double), cudaMemcpyHostToDevice));
  double t31 = gettime_ms();
  // printf("h2d time: %lf ms\n", t31 - t30);

  double t40 = gettime_ms();
  thrust::sort(thrust::device, d_vbuf.begin(), d_vbuf.end(), thrust::greater<double>());
  double t41 = gettime_ms();
  // printf("device sort time: %lf ms\n", t41 - t40);

  double t50 = gettime_ms();
  thrust::device_vector<int> d_values(nitems, 1);
  auto new_end = thrust::reduce_by_key(thrust::device,
    d_vbuf.begin(), d_vbuf.end(), d_values.begin(),
    thrust::make_discard_iterator(), d_values.begin());
  double t51 = gettime_ms();
  // printf("device reduce_by_key time: %lf ms\n", t51 - t50);

  // prints to stdout
  // thrust::copy(d_values.begin(), d_values.begin()+10, std::ostream_iterator<double>(std::cout, ", "));
  // printf("\n");

  thrust::device_vector<int>::iterator iter1 = d_values.begin();
  int values_size = thrust::distance(iter1, new_end.second);

  double t60 = gettime_ms();
  double entropy = thrust::transform_reduce(thrust::device, d_values.begin(), new_end.second, freq_calc(nitems), 0.0f, thrust::plus<double>());
  double t61 = gettime_ms();
  // printf("transform_reduce time: %lf ms\n", t61 - t60);

  double tend = gettime_ms();
  // printf("total time: %lf ms\n", tend - tstart);
  
  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  // printf("entropy: %f\n", entropy);
  printf("rank=%d, entropy=%f\n", rank, entropy);
}

int main(int argc, char* argv[])
{
  if(argc != 2)  fprintf(stderr, "usage: %s <file.bin>\n", argv[0]);
  const char *filename = argv[1];

  printf("reading file: %s\n", filename);

  uint8_t *bytes;
  // size_t len = 4;
  // size_t num_bytes = len*sizeof(float);

  size_t num_bytes = 0;
  double t0 = gettime_ms();
  readFileC(filename, &bytes, num_bytes);
  double t1 = gettime_ms();
  printf("read time: %lf ms\n", t1 - t0);
  size_t len = num_bytes / sizeof(float);

  float *buf = (float *)bytes;
  shannon_entropy(buf, len);

  return EXIT_SUCCESS;
}

size_t pdc_entropy_cpp(void *dataIn, pdc_var_type_t srcType, int ndim, uint64_t *dims, void **dataOut, pdc_var_type_t destType)
{
  FUNC_ENTER(NULL);
  // fprintf(stdout, "\n[TRANSFORM] pdc_entropy_cpp\n");

  // assert(srcType >= 0);
  // CUDA_RUNTIME_API_CALL( cudaSetDevice(0) );

  cudaStream_t stream;
  CUDA_RUNTIME_API_CALL(cudaStreamCreate(&stream));

  int len = 1;
  for (int i = 0; i < ndim; i++) {
    len *= dims[i];
  }

  void *buf = (void *)dataIn;

  switch (srcType)
  {
    case PDC_FLOAT:
      shannon_entropy((float *)buf, len);
      break;
    case PDC_DOUBLE:
      shannon_entropy_d((double *)buf, len);
      break;
    default:
      break;
  }

  destType = srcType;
  *dataOut = dataIn;

  FUNC_LEAVE(len);
  return len;
}

extern "C"
{
  size_t pdc_entropy(void *dataIn, pdc_var_type_t srcType, int ndim, uint64_t *dims, void **dataOut, pdc_var_type_t destType)
  {
    return pdc_entropy_cpp(dataIn, srcType, ndim, dims, dataOut, destType);
  }
}

