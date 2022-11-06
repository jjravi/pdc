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
  ((uint8_t)(access_type))
)

/**
 * @struct pdc_persist_in_t
 * @brief This is a struct
 */
MERCURY_GEN_PROC(pdc_persist_out_t, 
  ((int32_t)(ret)) /// return status code
)

hg_id_t pdc_persist_register();

#endif // PDC_SERVER_PERSIST_H
