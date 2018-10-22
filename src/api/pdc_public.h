/*
 * Copyright Notice for 
 * Proactive Data Containers (PDC) Software Library and Utilities
 * -----------------------------------------------------------------------------

 *** Copyright Notice ***
 
 * Proactive Data Containers (PDC) Copyright (c) 2017, The Regents of the
 * University of California, through Lawrence Berkeley National Laboratory,
 * UChicago Argonne, LLC, operator of Argonne National Laboratory, and The HDF
 * Group (subject to receipt of any required approvals from the U.S. Dept. of
 * Energy).  All rights reserved.
 
 * If you have questions about your rights to use or distribute this software,
 * please contact Berkeley Lab's Innovation & Partnerships Office at  IPO@lbl.gov.
 
 * NOTICE.  This Software was developed under funding from the U.S. Department of
 * Energy and the U.S. Government consequently retains certain rights. As such, the
 * U.S. Government has been granted for itself and others acting on its behalf a
 * paid-up, nonexclusive, irrevocable, worldwide license in the Software to
 * reproduce, distribute copies to the public, prepare derivative works, and
 * perform publicly and display publicly, and to permit other to do so.
 */

#ifndef _pdc_public_H
#define _pdc_public_H
#include <stdint.h>
#include <stdbool.h>

typedef int                         perr_t;
typedef uint64_t                    pdcid_t;
typedef unsigned long long          psize_t;
typedef bool                        pbool_t;

typedef int                         PDC_int_t;
typedef float                       PDC_float_t;
typedef double                      PDC_double_t;


perr_t PDCcont_put(const char *cont_name, pdcid_t *cont_id);
perr_t PDCcont_get_obj(const pdcid_t cont_id, const int *nobj, const pdcid_t **obj_ids);
perr_t PDCcont_add_obj(pdcid_t *cont_id, int nobj, pdcid_t *obj_ids);
perr_t PDCcont_delete(const pdcid_t cont_id);
perr_t PDCcont_put_tag(const pdcid_t cont_id, const char *tag_name, const void *tag_value, const uint64_t tag_size);
perr_t PDCcont_get_tag(const pdcid_t cont_id, void **tag_value, uint64_t *tag_size);
perr_t PDCcont_delete_tag(const pdcid_t cont_id, const char *tag_name);

perr_t PDCobj_put_data(const char *obj_name, const void *data, const uint64_t size, pdcid_t *obj_id);
perr_t PDCobj_get_data(const pdcid_t obj_id, const void **data, const uint64_t *size);
perr_t PDCobj_delete_data(const pdcid_t obj_id);
perr_t PDCobj_put_tag(const pdcid_t obj_id, const char *tag_name, const void *tag_value, const uint64_t tag_size);
perr_t PDCobj_get_tag(const pdcid_t obj_id, void **tag_value, uint64_t *tag_size);
perr_t PDCobj_delete_tag(const pdcid_t obj_id, const char *tag_name);
#endif
