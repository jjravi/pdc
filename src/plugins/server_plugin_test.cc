#include <stdio.h>

extern "C"
{
// #include "pdc_private.h"
#include "pdc_public.h"
#include "pdc_analysis.h"
}

int pdc_server_passthrough_cpp(pdcid_t iterIn, pdcid_t iterOut)
{
  printf("Entered: %s\n----------------\n", __func__);
  int  i, k, total = 0;
  int *dataIn  = NULL;
  int *dataOut = NULL;
  int blockLengthOut = PDCobj_data_getNextBlock(iterOut, (void **)&dataOut, NULL);
  printf("blockLengthOut: %d (%p)\n", blockLengthOut, dataOut);

  if (blockLengthOut > 0) {
    int blockLengthIn = PDCobj_data_getNextBlock(iterIn, (void **)&dataIn, NULL);
    do
    {
      printf("blockLengthIn: %d (%p)\n", blockLengthIn, dataIn);
      for (i = 0, k = 0; i < blockLengthIn; i++) {
        printf("%d, ", dataIn[i]);
        total += dataIn[i];
      }
      printf("\nSum = %d\n", total);
      dataOut[k++] = total;
      total        = 0;
      blockLengthIn = PDCobj_data_getNextBlock(iterIn, (void **)&dataIn, NULL);
    }
    while (blockLengthIn > 0);
  }

  printf("Leaving: %s\n----------------\n", __func__);
  return 0;
}

extern "C"
{
  int pdc_server_passthrough(pdcid_t iterIn, pdcid_t iterOut)
  {
    return pdc_server_passthrough_cpp(iterIn, iterOut);
  }
}

