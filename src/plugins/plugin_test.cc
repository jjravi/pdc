#include "pdc.h"
#include "pdc_private.h"

#include <stdio.h>

size_t pdc_passthrough_cpp(void *dataIn, pdc_var_type_t srcType, int ndim, uint64_t *dims, void **dataOut, pdc_var_type_t destType)
{
  FUNC_ENTER(NULL);

  size_t  typesize = 1;
  if (srcType >= 0) {
    if ((srcType == PDC_INT) || (srcType == PDC_UINT) || (srcType == PDC_FLOAT))
      typesize = sizeof(int);
    else if ((srcType == PDC_DOUBLE) || (srcType == PDC_INT64) || (srcType == PDC_UINT64))
      typesize = sizeof(double);
    else if (srcType == PDC_INT16)
      typesize = sizeof(short);
  }

  fprintf(stdout, "\n[TRANSFORM] pdc_passthrough\n");

  int nval = 1;
  for (int i = 0; i < ndim; i++) {
    nval *= dims[i];
  }

  int64_t nbytes   = nval * typesize;
  void *destBuff = malloc(nbytes);
  memcpy(destBuff, dataIn, nbytes);
  *dataOut = destBuff;

  fprintf(stdout, "\n[TRANSFORM] %d values successfully copied\n", nval);
  FUNC_LEAVE(nval);
  return nval;
}

extern "C"
{
  size_t pdc_passthrough(void *dataIn, pdc_var_type_t srcType, int ndim, uint64_t *dims, void **dataOut, pdc_var_type_t destType)
  {
    return pdc_passthrough_cpp(dataIn, srcType, ndim, dims, dataOut, destType);
  }
}

