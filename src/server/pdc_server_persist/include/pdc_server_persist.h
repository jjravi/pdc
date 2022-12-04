#include "rpc_engine.h"

#ifndef PDC_SERVER_PERSIST_H
#define PDC_SERVER_PERSIST_H

#include "pdc_client_server_common.h"

/**
 * @struct pdc_persist_in_t
 * @brief This is a struct
 */
MERCURY_GEN_PROC(pdc_persist_in_t,
  ((hg_bulk_t)(bulk_handle)) /// need for freeing.
  ((region_info_transfer_t)(remote_region))
  ((uint64_t)(obj_id))
  ((uint64_t)(buf_size))
  ((uint64_t)(obj_dim0))
  ((uint64_t)(obj_dim1))
  ((uint64_t)(obj_dim2))
  ((uint32_t)(remote_unit))
  ((int32_t)(obj_ndim))
  ((uint32_t)(meta_server_id))
  ((int32_t)(registered_op))
  ((uint8_t)(access_type))
  ((int32_t)(storageinfo))
)

  // TODO: jjravi delete registered_op

/*
 * The datatype isn't strictly needed but it can be nice
 * to have if we eventually provide a default 'fill value'.
 * This would be used when the server creates a temp in
 * place of a mapped region.
 * Generally, we assume that either a region is mapped
 * we haven't mapped because the object is an output
 * and we really don't care what the initial values are.
 * Note we package pdc_datatype into storage_info...
 * +--------+---------------+---------------+
 * |XXXXXXX | XXXXXXXXXXXXX | pdc_datatype  |
 * +---//---+---------------+---------------+
 * 31     16 15            8 7             0
 */

/**
 * @struct pdc_persist_in_t
 * @brief This is a struct
 */
MERCURY_GEN_PROC(pdc_persist_out_t,
  ((int32_t)(ret)) /// return status code
)

hg_id_t pdc_persist_register();

#endif // PDC_SERVER_PERSIST_H
