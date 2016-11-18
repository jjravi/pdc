#include "mercury.h"
#include "mercury_macros.h"
#include "mercury_proc_string.h"

#include "mercury_thread_pool.h"
#include "mercury_atomic.h"
#include "mercury_thread_mutex.h"
#include "mercury_hash_table.h"

#ifndef PDC_CLIENT_SERVER_COMMON_H
#define PDC_CLIENT_SERVER_COMMON_H

// For storing metadata
typedef struct hash_value_metadata_t {
    int obj_id;
    char obj_name[PATH_MAX];
    char obj_data_location[PATH_MAX];

} hash_value_metadata_t;


#ifdef HG_HAS_BOOST
MERCURY_GEN_PROC( gen_obj_id_in_t,  ((hg_const_string_t)(obj_name)) ((int32_t)(hash_value)) )
MERCURY_GEN_PROC( gen_obj_id_out_t, ((uint64_t)(ret)) )
#else
typedef struct {
    hg_const_string_t   obj_name;
    int32_t             hash_value;
} gen_obj_id_in_t;

typedef struct {
    uint64_t ret;
} gen_obj_id_out_t;

static HG_INLINE hg_return_t
hg_proc_gen_obj_id_in_t(hg_proc_t proc, void *data)
{
    hg_return_t ret;
    gen_obj_id_in_t *struct_data = (gen_obj_id_in_t*) data;

    ret = hg_proc_hg_const_string_t(proc, &struct_data->obj_name);
    if (ret != HG_SUCCESS) {
	HG_LOG_ERROR("Proc error");
    }
    ret = hg_proc_rpc_handle_t(proc, &struct_data->hash_value);
    if (ret != HG_SUCCESS) {
	HG_LOG_ERROR("Proc error");
    }
    return ret;
}


static HG_INLINE hg_return_t
hg_proc_gen_obj_id_out_t(hg_proc_t proc, void *data)
{
    hg_return_t ret;
    gen_obj_id_out_t *struct_data = (gen_obj_id_out_t*) data;

    ret = hg_proc_uint64_t(proc, &struct_data->ret);
    if (ret != HG_SUCCESS) {
	HG_LOG_ERROR("Proc error");
    }
    return ret;
}
#endif

hg_id_t gen_obj_id_register(hg_class_t *hg_class);

extern hg_hash_table_t *metadata_hash_table_g;


#endif /* PDC_CLIENT_SERVER_COMMON_H */