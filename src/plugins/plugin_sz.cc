// #include "pdc.h"
// #include "pdc_private.h"
extern "C"
{
#include "pdc_public.h"
#include "plugin_helper.h"
#include "pdc_utils.h"
}

#include "SZ3/api/sz.hpp"

#undef NDEBUG // enable asserts on release build
#include <assert.h>

#include <stdio.h>

#include <sys/time.h>
#include <time.h>
#include <unistd.h>

static double gettime_ms()
{
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC_RAW, &t);
  return (t.tv_sec+t.tv_nsec*1e-9)*1000;
}

template<class T>
void compress(char *inPath, char *cmpPath, SZ::Config conf) {
  T *data = new T[conf.num];
  SZ::readfile<T>(inPath, conf.num, data);

  size_t outSize;
  SZ::Timer timer(true);
  char *bytes = SZ_compress<T>(conf, data, outSize);
  double compress_time = timer.stop();

  char outputFilePath[1024];
  if (cmpPath == nullptr) {
    sprintf(outputFilePath, "%s.sz", inPath);
  } else {
    strcpy(outputFilePath, cmpPath);
  }
  SZ::writefile(outputFilePath, bytes, outSize);

  printf("compression ratio = %.2f \n", conf.num * 1.0 * sizeof(T) / outSize);
  printf("compression time = %f\n", compress_time);
  printf("compressed data file = %s\n", outputFilePath);

  delete[]data;
  delete[]bytes;
}

size_t pdc_sz_compress_cpp(void *dataIn, pdc_var_type_t srcType, int ndim, uint64_t *dims, void **dataOut, pdc_var_type_t destType)
{
  FUNC_ENTER(NULL);
  fprintf(stdout, "\n[TRANSFORM] pdc_sz_compress_cpp\n");

  assert(srcType >= 0);

  SZ::Config conf;

  switch(ndim)
  {
    case 1:
      conf = SZ::Config(dims[0]);
      break;
    case 2:
      conf = SZ::Config(dims[1], dims[0]);
      break;
    case 3:
      conf = SZ::Config(dims[2], dims[1], dims[0]);
      break;
    case 4:
      conf = SZ::Config(dims[3], dims[2], dims[1], dims[0]);
      break;
    default:
      assert(0);
      break;
  }

  // hack because SZ can't handle uniform data...
  {
    // float *hack_data = (float*)dataIn;
    // hack_data[0] = hack_data[0] * 0.1;
  }

  conf.loadcfg("sz3.config");

  void *bytes;
  size_t outSize;
  int nval = 1;
  nval = 1;
  for (int i = 0; i < ndim; i++) {
    nval *= dims[i];
  }
  int64_t nbytes = nval * PDC_get_var_type_size(srcType);

  double wanted_psnr = conf.psnrErrorBound;
  conf.num = nval;

  double c_start, c_end, d_start, d_end;

  double psnr=0, nrmse=0;
  // do
  // for(int ii = 0; ii < 10; ii++)
  {
    // conf.errorBoundMode = SZ::EB_REL;
    // conf.relErrorBound = conf.relErrorBound / 10;
    // conf.errorBoundMode = SZ::EB_ABS;
    // conf.absErrorBound = conf.absErrorBound / 10;
    conf.errorBoundMode = SZ::EB_PSNR;
    conf.psnrErrorBound = conf.psnrErrorBound - 0.1;

    if (*dataOut) free(*dataOut);

    psnr=0;
    nrmse=0;
    switch (srcType)
    {
      case PDC_FLOAT:
        c_start = gettime_ms();
        bytes = SZ_compress<float>(conf, (float *)dataIn, outSize);
        c_end = gettime_ms();
        break;
      case PDC_DOUBLE:
        c_start = gettime_ms();
        bytes = SZ_compress<double>(conf, (double *)dataIn, outSize);
        c_end = gettime_ms();
        break;
      default:
        assert(0); // TODO: jjravi support all PDC types
        break;
    }
    printf("c_time=%lf ms, c_throughput=%lf MB/s\n", (c_end-c_start), (nbytes*1e-6)/((c_end-c_start)*1e-3));

     *dataOut = bytes;

    // // TODO: jjravi remove temporary buffering here:
    void *destBuff = malloc(outSize);
    memcpy(destBuff, bytes, outSize);
    *dataOut = destBuff;
    // /////////////////////////////////////////////

    // print decompression results
    void *decData;
    switch (destType)
    {
      case PDC_FLOAT:
        d_start = gettime_ms();
        decData = SZ_decompress<float>(conf, (char *)destBuff, outSize);
        d_end = gettime_ms();
        break;
      case PDC_DOUBLE:
        d_start = gettime_ms();
        decData = SZ_decompress<double>(conf, (char *)destBuff, outSize);
        d_end = gettime_ms();
        break;
      default:
        assert(0); // TODO: jjravi support all PDC types
        break;
    }
    printf("d_time=%lf ms, d_throughput=%lf MB/s\n", (d_end-d_start), (outSize*1e-6)/((d_end-d_start)*1e-3));

    switch (destType)
    {
      case PDC_FLOAT:
        SZ::verify<float>((float *)dataIn, (float *)decData, nval, psnr, nrmse, false);
        break;
      case PDC_DOUBLE:
        SZ::verify<double>((double *)dataIn, (double *)decData, nval, psnr, nrmse, true);
        break;
      default:
        assert(0); // TODO: jjravi support all PDC types
        break;
    }

    printf("conf.psnrErrorBound: %lf, psnr: %lf\n****\n", conf.psnrErrorBound, psnr);
    // conf.psnrErrorBound = conf.psnrErrorBound - 0.1;
    // conf.errorBoundMode = SZ::EB_PSNR;
    // conf.errorBoundMode = SZ::EB_PSNR;
    // conf.absErrorBound = conf.absErrorBound - 0.01;
    // conf.relErrorBound = conf.relErrorBound - 0.01;
  }
  // while((int)psnr > (int)wanted_psnr);
 
  //////////////////////////////////////

  // fprintf(stdout, "\n[TRANSFORM] %d values successfully compressed\n", nval);

  fprintf(stdout, "\n[TRANSFORM] successfully compressed\n");
  fprintf(stdout, "\n[TRANSFORM] cratio=%lf, %ld bytes -> %ld bytes\n", (double)nbytes/(double)outSize, nbytes, outSize);

  // TODO: jjravi, change return type to be bytes instead of elements?
  // outSize = (size_t)( (float)outSize / (float)PDC_get_var_type_size(srcType) );

  FUNC_LEAVE(outSize);
  return outSize;
}

extern "C"
{
  size_t pdc_sz_compress(void *dataIn, pdc_var_type_t srcType, int ndim, uint64_t *dims, void **dataOut, pdc_var_type_t destType)
  {
    return pdc_sz_compress_cpp(dataIn, srcType, ndim, dims, dataOut, destType);
  }
}

