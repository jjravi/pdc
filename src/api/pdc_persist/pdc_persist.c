/**
  * Copyright Notice for
  * Proactive Data Containers (PDC) Software Library and Utilities
  * -----------------------------------------------------------------------------
  * Author: John Ravi jjravi@ncsu.edu
  * This file is the core I/O module of the PDC region transfer at the client side.
  * -----------------------------------------------------------------------------

  *** Copyright Notice ***

  * Proactive Data Containers (PDC) Copyright (c) 2022, The Regents of the
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
 **/

#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include "pdc_utlist.h"
#include "pdc_config.h"
#include "pdc_id_pkg.h"
#include "pdc_obj.h"
#include "pdc_obj_pkg.h"
#include "pdc_malloc.h"
#include "pdc_prop_pkg.h"
#include "pdc_region.h"
#include "pdc_region_pkg.h"
#include "pdc_obj_pkg.h"
#include "pdc_interface.h"
#include "pdc_transforms_pkg_old.h"
#include "pdc_client_connect.h"
#include "pdc_analysis_pkg.h"
#include <mpi.h>

#include "pdc_persist.h"

/*
 * This function binds a transfer request to its corresponding object, so the object is aware of any ongoing
 * region transfer operation. Called when transfer request start is executed. Why do we do this? Sometimes
 * users may close an object without calling transfer request wait, so it is our responsibility to wait for
 * the request at the object's close time.
 */
static perr_t
attach_local_transfer_request(struct _pdc_obj_info *p, pdcid_t transfer_request_id)
{
    perr_t ret_value = SUCCEED;

    FUNC_ENTER(NULL);
    if (p->local_transfer_request_head != NULL) {
        p->local_transfer_request_end->next =
            (pdc_local_transfer_request *)malloc(sizeof(pdc_local_transfer_request));
        p->local_transfer_request_end       = p->local_transfer_request_end->next;
        p->local_transfer_request_end->next = NULL;
    }
    else {
        p->local_transfer_request_head =
            (pdc_local_transfer_request *)malloc(sizeof(pdc_local_transfer_request));
        p->local_transfer_request_end       = p->local_transfer_request_head;
        p->local_transfer_request_end->next = NULL;
    }
    p->local_transfer_request_end->local_id = transfer_request_id;
    p->local_transfer_request_size++;
    fflush(stdout);
    FUNC_LEAVE(ret_value);
}

/*
 * Pack user memory buffer into a contiguous buffer based on local region shape.
 */
static perr_t
pack_region_buffer(char *buf, uint64_t *obj_dims, size_t total_data_size, int local_ndim,
                   uint64_t *local_offset, uint64_t *local_size, size_t unit, pdc_access_t access_type,
                   char **new_buf)
{
    uint64_t i, j;
    perr_t   ret_value = SUCCEED;
    char *   ptr;

    FUNC_ENTER(NULL);

    if (local_ndim == 1) {
        /*
                printf("checkpoint at local copy ndim == 1 local_offset[0] = %lld @ line %d\n",
                       (long long int)local_offset[0], __LINE__);
        */
        *new_buf = buf + local_offset[0] * unit;
    }
    else if (local_ndim == 2) {
        *new_buf = (char *)malloc(sizeof(char) * total_data_size);
        if (access_type == PDC_WRITE) {
            ptr = *new_buf;
            for (i = 0; i < local_size[0]; ++i) {
                memcpy(ptr, buf + ((local_offset[0] + i) * obj_dims[1] + local_offset[1]) * unit,
                       local_size[1] * unit);
                ptr += local_size[1] * unit;
            }
        }
    }
    else if (local_ndim == 3) {
        *new_buf = (char *)malloc(sizeof(char) * total_data_size);
        if (access_type == PDC_WRITE) {
            ptr = *new_buf;
            for (i = 0; i < local_size[0]; ++i) {
                for (j = 0; j < local_size[1]; ++j) {
                    memcpy(ptr,
                           buf + ((local_offset[0] + i) * obj_dims[1] * obj_dims[2] +
                                  (local_offset[1] + j) * obj_dims[2] + local_offset[2]) *
                                     unit,
                           local_size[2] * unit);
                    ptr += local_size[2] * unit;
                }
            }
        }
    }
    else {
        ret_value = FAIL;
    }

    fflush(stdout);
    FUNC_LEAVE(ret_value);
}

perr_t pdcTransferStart(pdcid_t transfer_request_id)
{
  perr_t                ret_value = SUCCEED;
  struct _pdc_id_info * transferinfo;
  pdc_transfer_request *transfer_request;
  size_t                unit;
  int                   i;

  FUNC_ENTER(NULL);

  transferinfo = PDC_find_id(transfer_request_id);

  transfer_request = (pdc_transfer_request *)(transferinfo->obj_ptr);

  if (transfer_request->metadata_id != NULL) {
    printf("PDC Client PDCregion_transfer_start attempt to start existing transfer request @ line %d\n",
      __LINE__);
    ret_value = FAIL;
    goto done;
  }

  // Dynamic case is implemented within the the aggregated version. The main reason is that the target data
  // server may not be unique, so we may end up sending multiple requests to the same data server.
  // Aggregated method will take care of this type of operation.
  assert(transfer_request->region_partition == PDC_OBJ_STATIC);

  attach_local_transfer_request(transfer_request->obj_pointer, transfer_request_id);

  // Pack local region to a contiguous memory buffer
  unit = transfer_request->unit;

  // Convert user buf into a contiguous buffer called new_buf, which is determined by the shape of local
  // objects.
  pack_region_buffer(transfer_request->buf, transfer_request->obj_dims, transfer_request->total_data_size,
    transfer_request->local_region_ndim, transfer_request->local_region_offset,
    transfer_request->local_region_size, unit, transfer_request->access_type,
    &(transfer_request->new_buf));


  // else if (transfer_request->region_partition == PDC_OBJ_STATIC)
  {
    // Static object partitioning means that all requests for the same object are sent to the same data
    // server.
    transfer_request->metadata_id   = (uint64_t *)malloc(sizeof(uint64_t));
    transfer_request->n_obj_servers = 1;
    if (transfer_request->access_type == PDC_READ) {
      transfer_request->read_bulk_buf =
        (char **)malloc(sizeof(char *) * transfer_request->n_obj_servers);
      transfer_request->read_bulk_buf[0] = transfer_request->new_buf;
    }
    // Submit transfer request to server by designating data server ID, remote region info, and contiguous
    // memory buffer for copy.
    ret_value = PDC_Client_transfer_request(
      transfer_request->new_buf, transfer_request->obj_id, transfer_request->data_server_id,
      transfer_request->obj_ndim, transfer_request->obj_dims, transfer_request->remote_region_ndim,
      transfer_request->remote_region_offset, transfer_request->remote_region_size, unit,
      transfer_request->access_type, transfer_request->metadata_id);
  }
done:
  FUNC_LEAVE(ret_value);
}

perr_t pdcTransferWait(pdcid_t transfer_request_id)
{

}

