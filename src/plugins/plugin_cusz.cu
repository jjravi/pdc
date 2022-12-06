// #include "pdc.h"
// #include "pdc_private.h"
extern "C"
{
#include "pdc_public.h"
#include "plugin_helper.h"
}


#undef NDEBUG // enable asserts on release build
#include <assert.h>

#include <stdio.h>

#include "api.hh"

#include "cli/quality_viewer.hh"
#include "cli/timerecord_viewer.hh"
#include "utils/autotune.cuh"

#include <cuda.h>

#define CUDA_RUNTIME_API_CALL(apiFuncCall)                               \
{                                                                        \
  cudaError_t _status = apiFuncCall;                                     \
  if (_status != cudaSuccess) {                                          \
    fprintf(stderr, "%s:%d: error: function %s failed with error %s.\n", \
      __FILE__, __LINE__, #apiFuncCall, cudaGetErrorString(_status));    \
    exit(-1);                                                            \
  }                                                                      \
}

bool is_device_pointer(const void *ptr)
{
  struct cudaPointerAttributes attributes;
  cudaPointerGetAttributes(&attributes, ptr);
  return (attributes.devicePointer != NULL);
}

void compress_array(void *buf, int ndim, uint64_t *dims, size_t size)
{
  using T = float;
  Capsule<T> input("uncompressed");

  {
    int len = 1;
    for (int i = 0; i < ndim; i++) {
      len *= dims[i];
    }

    /* cuSZ requires a 3% overhead on device (not required on host). */
    size_t uncompressed_alloclen = len * 1.03;
    size_t decompressed_alloclen = uncompressed_alloclen;

    /* code snippet for looking at the device array easily */
    auto peek_devdata = [](T* d_arr, size_t num = 20) {
      thrust::for_each(thrust::device, d_arr, d_arr + num, [=] __device__ __host__(const T i) { printf("%f\t", i); });
      printf("\n");
    };

    T *d_uncompressed;
    CUDA_RUNTIME_API_CALL(cudaMalloc(&d_uncompressed, sizeof(T) * uncompressed_alloclen ));
    CUDA_RUNTIME_API_CALL(cudaMemcpy(d_uncompressed, buf, size, cudaMemcpyDeviceToDevice));

    /* a casual peek */
    printf("peeking uncompressed data, 20 elements\n");
    peek_devdata(d_uncompressed, 20);

    cudaStream_t stream;
    CUDA_RUNTIME_API_CALL(cudaStreamCreate(&stream));

    using Compressor = typename cusz::Framework<T>::LorenzoFeaturedCompressor;
    Compressor*  compressor = new Compressor;
    BYTE* exposed_compressed;
    size_t compressed_len;
    {
      cusz::TimeRecord timerecord;
      cusz::Context* ctx = new cusz::Context();
      ctx->set_len(dims[0], 1, 1, 1)
        .set_eb(47803.025242)                      // numeric
        .set_control_string("mode=r2r");  // string

      cusz::Header header;
      cusz::core_compress(
          compressor, ctx,                             // compressor & config
          d_uncompressed, uncompressed_alloclen,       // input
          exposed_compressed, compressed_len, header,  // output
          stream, &timerecord);

      /* User can interpret the collected time information in other ways. */
      cusz::TimeRecordViewer::view_compression(&timerecord, len * sizeof(T), compressed_len);

      /* verify header */
      printf("header.%-*s : %x\n",            12, "(addr)", &header);
      printf("header.%-*s : %lu, %lu, %lu\n", 12, "{x,y,z}", header.x, header.y, header.z);
      printf("header.%-*s : %lu\n",           12, "filesize", header.get_filesize());
    }

    printf("compression done\n");
    printf("compressed_len: %ld\n", compressed_len);

    /* If needed, User should perform a memcopy to transfer `exposed_compressed` before `compressor` is destroyed. */
    BYTE* compressed;
    CUDA_RUNTIME_API_CALL(cudaMalloc(&compressed, compressed_len));
    CUDA_RUNTIME_API_CALL(cudaMemcpy(compressed, exposed_compressed, compressed_len, cudaMemcpyDeviceToDevice));
  }
}

size_t pdc_cusz_compress_cpp(void *dataIn, pdc_var_type_t srcType, int ndim, uint64_t *dims, void **dataOut, pdc_var_type_t destType)
{
  FUNC_ENTER(NULL);
  fprintf(stdout, "\n[TRANSFORM] pdc_sz_compress_cpp\n");

  assert(srcType >= 0);
  // TODO: jjravi, if mptr is in host memory

  // CUDA_RUNTIME_API_CALL( cudaSetDevice(0) );

  cudaStream_t stream;
  CUDA_RUNTIME_API_CALL(cudaStreamCreate(&stream));

  // CUcontext pctx;
  // cuCtxGetCurrent(&pctx);
  // printf("pctx: %p\n", pctx);

  int len = 1;
  for (int i = 0; i < ndim; i++) {
    len *= dims[i];
  }

  // compress_array(dataIn, ndim, dims, len*sizeof(float));

  // code snippet for looking at the device array easily
  using T = float;
  auto peek_devdata = [](T* d_arr, size_t num = 20) {
    thrust::for_each(thrust::device, d_arr, d_arr + num, [=] __device__ __host__(const T i) { printf("%f\t", i); });
    printf("\n");
  };

  // TODO: jjravi UVM is slower than device memory
  // float *d_uncompressed;
  // d_uncompressed = (float *)dataIn;
  printf("is_device_pointer: %d\n", is_device_pointer(dataIn));

  // TODO: and compressor needs 3% oveerhead?
  T *d_uncompressed;
  // cuSZ requires a 3% overhead on device (not required on host).
  size_t uncompressed_alloclen = len * 1.03;
  size_t decompressed_alloclen = uncompressed_alloclen;
  CUDA_RUNTIME_API_CALL(cudaMalloc(&d_uncompressed, sizeof(T) * uncompressed_alloclen));
  // CUDA_RUNTIME_API_CALL(cudaMemcpy(d_uncompressed, dataIn, sizeof(T) * len, cudaMemcpyDeviceToDevice));
  CUDA_RUNTIME_API_CALL(cudaMemcpy(d_uncompressed, dataIn, sizeof(T) * len, cudaMemcpyHostToDevice));

  // printf("peeking uncompressed data, 20 elements\n");
  // peek_devdata(d_uncompressed, 20);

  using Compressor = typename cusz::Framework<T>::LorenzoFeaturedCompressor;
  Compressor*  compressor = new Compressor;
  BYTE* exposed_compressed;

  // const float eb = 47803.025242;
  // const float eb = 0.00001;
  const double eb = 1e-2;

  cusz::Context* ctx = new cusz::Context();
  switch(ndim)
  {
    case 1:
      printf("1d data\n");
      ctx->set_len(dims[0], 1, 1, 1)
        .set_eb(eb)                      // numeric
        .set_control_string("mode=r2r");  // string

      AutoconfigHelper::autotune(ctx);

      ctx->set_eb(eb);
      ctx->set_control_string("mode=r2r");
      ctx->verbose = true;
      ctx->mode = "r2r";
      ctx->eb = eb;
      break;
    case 2:
      printf("2d data\n");
      ctx->set_len(dims[0], dims[1], 1, 1)
        .set_eb(eb)                      // numeric
        .set_control_string("mode=abs");  // string
      break;
    case 3:
      printf("3d data\n");
      ctx->set_len(dims[0], dims[1], dims[2], 1)
        .set_eb(eb)                      // numeric
        .set_control_string("mode=abs");  // string
      break;
    case 4:
      printf("4d data\n");
      ctx->set_len(dims[0], dims[1], dims[2], dims[3])
        .set_eb(eb)                      // numeric
        .set_control_string("mode=abs");  // string
      break;
    default:
      assert(0);
      break;
  }

  cusz::TimeRecord timerecord;
  size_t compressed_len;
  cusz::Header header;

  {
    nvtxRangePush("core_compress");

    cusz::core_compress(
      compressor, ctx,                           // compressor & config
      d_uncompressed, uncompressed_alloclen,     // input
      exposed_compressed, compressed_len, header,  // output
      stream, &timerecord
    );

    nvtxRangePop();
  }


  {
    nvtxRangePush("view_compression");

    // User can interpret the collected time information in other ways
    cusz::TimeRecordViewer::view_compression(&timerecord, len * sizeof(T), compressed_len);

    printf("header.%-*s : %x\n",            12, "(addr)", &header);
    printf("header.%-*s : %lu, %lu, %lu\n", 12, "{x,y,z}", header.x, header.y, header.z);
    printf("header.%-*s : %lu\n",           12, "filesize", header.get_filesize());


    nvtxRangePop();
  }



  // *dataOut = bytes;

  // CUDA_RUNTIME_API_CALL(cudaMallocManaged((void **)&dataOut, compressed_len));

  // TODO: temporarily allocate the full amount
  
  CUDA_RUNTIME_API_CALL(cudaMallocManaged(dataOut, compressed_len));
  CUDA_RUNTIME_API_CALL(cudaMemcpy(*dataOut, exposed_compressed, compressed_len, cudaMemcpyDeviceToDevice));
  size_t outSize = compressed_len;

  fprintf(stdout, "\n[TRANSFORM] successfully compressed\n");
  fprintf(stdout, "\n[TRANSFORM] %ld bytes -> %ld bytes\n", len*sizeof(T), outSize);

  // TODO: jjravi, change return type to be bytes instead of elements?
  // outSize = (size_t)( (float)outSize / (float)PDC_get_var_type_size(srcType) );

  FUNC_LEAVE(outSize);
  return outSize;
}

extern "C"
{
  size_t pdc_cusz_compress(void *dataIn, pdc_var_type_t srcType, int ndim, uint64_t *dims, void **dataOut, pdc_var_type_t destType)
  {
    return pdc_cusz_compress_cpp(dataIn, srcType, ndim, dims, dataOut, destType);
  }
}

