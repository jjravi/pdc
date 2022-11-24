#ifndef PDC_SERVER_TRANSFORM_H
#define PDC_SERVER_TRANSFORM_H

#include "rpc_engine.h"

// #define NAME_MAX   (255)

struct factory_payload
{
  size_t func_name_length;
  size_t file_name_length;
  size_t code_size;

  char func_name[NAME_MAX];
  char file_name[NAME_MAX];
  void *code_binary;
};

/**
 * @struct pdc_transform_in_t
 * @brief This is a struct
 */
MERCURY_GEN_PROC(pdc_transform_in_t,
  ((hg_bulk_t)(bulk_handle)) /// need for freeing.
  ((uint64_t)(obj_id))
  ((uint64_t)(buf_size))
)

/**
 * @struct pdc_transform_in_t
 * @brief This is a struct
 */
MERCURY_GEN_PROC(pdc_transform_out_t,
  ((int64_t)(ftn_addr)) /// transform address 
  ((int32_t)(ret))       /// return status code
)

hg_id_t pdc_transform_register();

#endif // PDC_SERVER_TRANSFORM_H
