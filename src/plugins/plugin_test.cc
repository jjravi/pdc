#define ENABLE_MULTITHREAD
// #include "pdc.h"
#include "mercury_atomic.h"
extern "C"
{
#include "pdc_client_server_common.h"
#include "pdc_server_data.h"
#include "pdc_public.h"
#include "pdc_interface.h"
#include "plugin_helper.h"
}

#include "pdc_id_pkg.h"

#include <cstdio>
#include <cassert>

size_t pdc_passthrough_cpp(void *dataIn, pdc_var_type_t srcType, int ndim, uint64_t *dims, void **dataOut, pdc_var_type_t destType, pdcid_t id)
{
  FUNC_ENTER(NULL);

  // fprintf(stdout, "[TRANSFORM] id: %lu\n", id);
  // data_server_region_t *region         = NULL;
  // region = PDC_Server_get_obj_region(id);
  // PDC_Server_register_obj_region_by_pointer(&region, id, 0);
  // fprintf(stdout, "[TRANSFORM] region: %p\n", region);

  // struct _pdc_id_info *info = nullptr;
  // info = PDC_find_id(id);
  // assert(info != nullptr && info != NULL); //cannot locate object property ID
  // struct _pdc_obj_prop *props = (struct _pdc_obj_prop *)info->obj_ptr;
 
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
  size_t pdc_passthrough(void *dataIn, pdc_var_type_t srcType, int ndim, uint64_t *dims, void **dataOut, pdc_var_type_t destType, pdcid_t id)
  {
    return pdc_passthrough_cpp(dataIn, srcType, ndim, dims, dataOut, destType, id);
  }
}


// void pdc_passthrough_cpp(void **dataIn, pdc_var_type_t *srcType, int *ndim, uint64_t **dims, void **dataOut, size_t* data_size)
// {
//   FUNC_ENTER(NULL);
// 
//   // struct _pdc_id_info *info = nullptr;
//   // info = PDC_find_id(id);
//   // assert(info != nullptr && info != NULL); //cannot locate object property ID
//   // struct _pdc_obj_prop *props = (struct _pdc_obj_prop *)info->obj_ptr;
//  
//   size_t  typesize = 1;
//   if (srcType >= 0) {
//     if ((srcType == PDC_INT) || (srcType == PDC_UINT) || (srcType == PDC_FLOAT))
//       typesize = sizeof(int);
//     else if ((srcType == PDC_DOUBLE) || (srcType == PDC_INT64) || (srcType == PDC_UINT64))
//       typesize = sizeof(double);
//     else if (srcType == PDC_INT16)
//       typesize = sizeof(short);
//   }
// 
//   fprintf(stdout, "\n[TRANSFORM] pdc_passthrough\n");
// 
//   int nval = 1;
//   for (int i = 0; i < ndim; i++) {
//     nval *= dims[i];
//   }
// 
//   int64_t nbytes   = nval * typesize;
//   void *destBuff = malloc(nbytes);
//   memcpy(destBuff, dataIn, nbytes);
//   *dataOut = destBuff;
// 
//   fprintf(stdout, "\n[TRANSFORM] %d values successfully copied\n", nval);
//   data_size = nval;
//   FUNC_LEAVE(NULL);
// }
// 
// extern "C"
// {
//   void pdc_passthrough(void **dataIn, pdc_var_type_t *srcType, int *ndim, uint64_t **dims, void **dataOut, size_t* data_size)
//   {
//     return pdc_passthrough_cpp(dataIn, srcType, ndim, dims, dataOut, data_size);
//   }
// }

