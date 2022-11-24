#ifndef PDC_TRANSFORM_H
#define PDC_TRANSFORM_H

#include "pdc_public.h"
#include "pdc_obj.h"

perr_t pdcTransformRegionRegister(char *func, pdcid_t region_id);
perr_t pdcTransformObjectRegister(char *func, pdcid_t object_id);

#endif /* PDC_TRANSFORM_H */

