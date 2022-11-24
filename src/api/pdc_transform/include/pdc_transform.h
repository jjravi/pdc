#ifndef PDC_TRANSFORM_H
#define PDC_TRANSFORM_H

#include "pdc_public.h"
#include "pdc_obj.h"

typedef enum
{
  PDC_COMPUTE_CPU = 0,
  PDC_COMPUTE_GPU = 1,
  PDC_COMPUTE_DPU = 2,
  PDC_COMPUTE_UNKNOWN = 3
} pdc_compute_variant_exec_t;

perr_t pdcTransformRegionRegister(char* name, char *func, pdcid_t region_id, pdc_compute_variant_exec_t executor);

perr_t pdcTransformObjectRegister(char* name, char *func, pdcid_t object_id, pdc_compute_variant_exec_t executor);

typedef enum
{
  PSNR = 0,
  ABS = 1
} pdc_compression_error_bound_t;

// perr_t PDCprop_set_obj_error_range(obj_prop_qrain, 40, 80, PSNR);
perr_t PDCprop_set_obj_error_range(pdcid_t object_id, int low, int high, pdc_compression_error_bound_t eb_type);


#endif /* PDC_TRANSFORM_H */

