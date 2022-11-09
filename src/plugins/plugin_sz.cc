// #include "pdc.h"
// #include "pdc_private.h"
extern "C"
{
#include "pdc_public.h"
#include "plugin_helper.h"
}

#include "SZ3/api/sz.hpp"

#undef NDEBUG // enable asserts on release build
#include <assert.h>

#include <stdio.h>

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

  conf.loadcfg("sz3.config");

  void *bytes;
  size_t outSize;

  switch (srcType)
  {
    case PDC_FLOAT:
      bytes = SZ_compress<float>(conf, (float *)dataIn, outSize);
      break;
    default:
      assert(0); // TODO: jjravi support all PDC types
      break;
  }

  int nval = 1;
  for (int i = 0; i < ndim; i++) {
    nval *= dims[i];
  }
  int64_t nbytes = nval * PDC_get_var_type_size(srcType);
  *dataOut = bytes;

  // // TODO: jjravi remove temporary buffering here:
  // void *destBuff = malloc(outSize);
  // memcpy(destBuff, bytes, outSize);
  // *dataOut = destBuff;
  // /////////////////////////////////////////////

  // fprintf(stdout, "\n[TRANSFORM] %d values successfully compressed\n", nval);

  fprintf(stdout, "\n[TRANSFORM] successfully compressed\n");
  fprintf(stdout, "\n[TRANSFORM] %ld bytes -> %ld bytes\n", nbytes, outSize);

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

