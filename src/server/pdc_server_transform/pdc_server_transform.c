#include "pdc_server_transform.h"
#include <aio.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "pdc_private.h"
#include "pdc_interface.h"
#include "pdc_id_pkg.h"
#include "pdc_obj_pkg.h"
#include "pdc_prop_pkg.h"
#include "pdc_utlist.h"
#include "pdc_client_server_common.h"

#define HG_API_CALL(apiFuncCall)                                         \
{                                                                        \
  hg_return_t _status = apiFuncCall;                                     \
  if (_status != HG_SUCCESS) {                                           \
    fprintf(stderr, "%s:%d: error: function %s failed with error %s.\n", \
      __FILE__, __LINE__, #apiFuncCall, HG_Error_to_string(_status));    \
    exit(-1);                                                            \
  }                                                                      \
}

extern int data_sieving_g;

/// struct used to carry state of overall operation across callbacks
struct pdc_transform_state {
  hg_size_t payload_size;
  void *opaque_ptr;
  hg_bulk_t bulk_handle;
  hg_handle_t handle;
  pdc_transform_in_t in;
};

/// callback triggered upon completion of async write
static void pdc_transform_handler_write_cb(union sigval sig)
{
  FUNC_ENTER(NULL);
  struct pdc_transform_state *pdc_transform_state_p = sig.sival_ptr;
  // int ret = aio_error(&pdc_transform_state_p->acb);
  // assert(ret == 0);

  pdc_transform_out_t out;
  out.ret = 0;

  // NOTE: really this should be nonblocking
  // close(pdc_transform_state_p->acb.aio_fildes);

  // send ack to client
  // NOTE: don't bother specifying a callback here for completion of sending response. This is just a best effort response.
  HG_API_CALL(HG_Respond(pdc_transform_state_p->handle, NULL, NULL, &out));

  HG_Bulk_free(pdc_transform_state_p->bulk_handle);
  HG_Destroy(pdc_transform_state_p->handle);
  // free(pdc_transform_state_p->payload);
  free(pdc_transform_state_p);

  FUNC_LEAVE(NULL);
  return;
}

/// callback triggered upon completion of bulk transfer
static hg_return_t pdc_transform_handler_bulk_cb(const struct hg_cb_info *info)
{
  FUNC_ENTER(NULL);
  assert(info->ret == 0);
  struct pdc_transform_state *pdc_transform_state_p = info->arg;

  struct factory_payload *payload = (struct factory_payload*)pdc_transform_state_p->opaque_ptr;

  ///////////////////////////////
  // printf("payload->func_name: %s\n", payload->func_name);
  // printf("payload->func_name_length: %ld\n", payload->func_name_length);
  // printf("payload->file_name: %s\n", payload->file_name);
  // printf("payload->file_name_length: %ld\n", payload->file_name_length);
  // printf("payload->code_size: %ld\n", payload->code_size);

  char tmp[PATH_MAX] = {};
  // snprintf(tmp, sizeof(tmp), "/tmp/%s", transformslibrary);
  snprintf(tmp, sizeof(tmp), "/dev/shm/%s", payload->file_name);
  // printf("write to %s\n", tmp);
  if(!access(tmp, R_OK)) unlink(tmp);
  int fd = open(tmp, O_CREAT|O_WRONLY|O_TRUNC, 0755);
  size_t n = write(fd, pdc_transform_state_p->opaque_ptr+sizeof(size_t)*3+NAME_MAX*2, payload->code_size);
  close(fd);
  //////////////////////////

  // 3. server callback dlopen and dlsym
  void *appHandle = NULL;

  if( (appHandle = dlopen(tmp, RTLD_NOW)) == NULL)
  {
    fprintf(stderr, "dlopen failed: %s", dlerror());
  }
  // print_symbols();

  // 4. returns function address to client.
  void *ftnHandle = NULL;

  if ( (ftnHandle = dlsym(appHandle, payload->func_name)) == NULL)
  {
    fprintf(stderr, "dlsym failed: %s", dlerror());
  }
  printf("function address: %p\n", ftnHandle);

  /*
  {
    struct pdc_region_info *remote_reg_info;
    uint64_t obj_dims[3];
    remote_reg_info = (struct pdc_region_info *)malloc(sizeof(struct pdc_region_info));
    remote_reg_info->ndim   = (pdc_transform_state_p->in.remote_region).ndim;
    remote_reg_info->offset = (uint64_t *)malloc(remote_reg_info->ndim * sizeof(uint64_t));
    remote_reg_info->dims_size   = (uint64_t *)malloc(remote_reg_info->ndim * sizeof(uint64_t));

    // TODO: jjravi replace with for loop
    if (remote_reg_info->ndim >= 1) {
      (remote_reg_info->offset)[0] = (pdc_transform_state_p->in.remote_region).start_0;
      (remote_reg_info->dims_size)[0]   = (pdc_transform_state_p->in.remote_region).count_0;
      obj_dims[0]                  = (pdc_transform_state_p->in).obj_dim0;
    }
    if (remote_reg_info->ndim >= 2) {
      (remote_reg_info->offset)[1] = (pdc_transform_state_p->in.remote_region).start_1;
      (remote_reg_info->dims_size)[1]   = (pdc_transform_state_p->in.remote_region).count_1;
      obj_dims[1]                  = (pdc_transform_state_p->in).obj_dim1;
    }
    if (remote_reg_info->ndim >= 3) {
      (remote_reg_info->offset)[2] = (pdc_transform_state_p->in.remote_region).start_2;
      (remote_reg_info->dims_size)[2]   = (pdc_transform_state_p->in.remote_region).count_2;
      obj_dims[2]                  = (pdc_transform_state_p->in).obj_dim2;
    }
    /////////
    remote_reg_info->unit = pdc_transform_state_p->in.remote_unit;
    remote_reg_info->data_buf = pdc_transform_state_p->data_buffer;

    // TODO: region size is different from bytes (compression)
    remote_reg_info->data_size = pdc_transform_state_p->in.buf_size;
    // remote_reg_info->data_size = remote_reg_info->unit * remote_reg_info->dims_size[0];
    for(size_t i = 1; i < remote_reg_info->ndim; i++)
    {
      remote_reg_info->data_size *= remote_reg_info->dims_size[i];
    }

    PDC_Server_transfer_request_io(
      pdc_transform_state_p->in.obj_id,
      pdc_transform_state_p->in.obj_ndim,
      obj_dims,
      remote_reg_info, 1
    );
  }
  */

  /////////////////////////////////////////
  data_server_region_t *region         = NULL;
  // uint64_t write_size = pdc_transform_state_p->in.data_size;

  region = PDC_Server_get_obj_region(pdc_transform_state_p->in.obj_id);
  PDC_Server_register_obj_region_by_pointer(&region, pdc_transform_state_p->in.obj_id, 0);
  region->obj_transform_enable = TRANSFORM_ENABLE_MAGIC;
  region->transform_ftn_addr = ftnHandle;

  /////////////////////////////////////////
  {
    pdc_transform_out_t out;
    out.ftn_addr = (int64_t)ftnHandle;
    out.ret = 0;
    HG_API_CALL(HG_Respond(pdc_transform_state_p->handle, NULL, NULL, &out));
    HG_Bulk_free(pdc_transform_state_p->bulk_handle);
    HG_Destroy(pdc_transform_state_p->handle);
    free(pdc_transform_state_p->opaque_ptr);
    free(pdc_transform_state_p);
  }

  HG_API_CALL(HG_Bulk_free(pdc_transform_state_p->in.bulk_handle));
  HG_API_CALL(HG_Destroy(pdc_transform_state_p->handle));

  FUNC_LEAVE(0);
  return 0;
}

/// callback/handler triggered upon receipt of rpc request
static hg_return_t pdc_transform_handler(hg_handle_t handle)
{
  // TODO: should do the same as pdc_client_server_common.c:724
  // transfer_request, handle
  FUNC_ENTER(NULL);
  // set up state structure
  struct pdc_transform_state *pdc_transform_state_p = malloc(sizeof(*pdc_transform_state_p));
  assert(pdc_transform_state_p);
  pdc_transform_state_p->handle = handle;

  // decode input
  HG_API_CALL(HG_Get_input(handle, &pdc_transform_state_p->in));

  pdc_transform_state_p->payload_size = pdc_transform_state_p->in.buf_size;
  // pdc_transform_state_p->payload = malloc(pdc_transform_state_p->in.buf_size);
  // assert(pdc_transform_state_p->payload);
  pdc_transform_state_p->opaque_ptr = malloc(pdc_transform_state_p->in.buf_size);
  assert(pdc_transform_state_p->opaque_ptr);

  printf("Got RPC request with buf_size: %ld for obj_id: %lu\n", pdc_transform_state_p->in.buf_size, pdc_transform_state_p->in.obj_id);

  // printf("region: %lu\n", region);
  // printf("region->obj_id: %lu\n", region->obj_id);

  // struct _pdc_id_info *objinfo1 = NULL; 
  // printf("derefering now:\n");
  // objinfo1 = PDC_find_id((pdcid_t)pdc_transform_state_p->in.obj_id);
  // printf("derefering done:\n");
  // if (objinfo1 == NULL)
  // {
  //   fprintf(stderr, "cannot locate object ID: %ld", pdc_transform_state_p->in.obj_id);
  // }
  // else
  // {
  //   struct _pdc_obj_info *obj1 = NULL;
  //   obj1 = (struct _pdc_obj_info *)(objinfo1->obj_ptr);
  //   if (obj1)
  //   {
  //     printf("object found!\n");
  //     // printf("object 1 properties, time_step: %d\n", obj1->obj_pt, obj1->obj_pt->time_step);
  //   }
  // }

  // register local target buffer for bulk access
  const struct hg_info *hgi = HG_Get_info(handle);
  assert(hgi);
  int ret = HG_Bulk_create(hgi->hg_class, 1, &pdc_transform_state_p->opaque_ptr, &pdc_transform_state_p->payload_size,
    HG_BULK_WRITE_ONLY, &pdc_transform_state_p->bulk_handle);
  assert(ret == 0);

  // initiate bulk transfer from client to server
  ret = HG_Bulk_transfer(hgi->context, pdc_transform_handler_bulk_cb, pdc_transform_state_p,
    HG_BULK_PULL, hgi->addr, pdc_transform_state_p->in.bulk_handle, 0,
    pdc_transform_state_p->bulk_handle, 0, pdc_transform_state_p->payload_size, HG_OP_ID_IGNORE);
  assert(ret == 0);

  FUNC_LEAVE(0);
  return 0;
}

/// register pdc_transform type with Mercury
hg_id_t pdc_transform_register()
{
  FUNC_ENTER(NULL);
  hg_class_t *hg_class = hg_engine_get_class();
  hg_id_t tmp = MERCURY_REGISTER(hg_class, "pdc_transform", pdc_transform_in_t, pdc_transform_out_t, pdc_transform_handler);
  FUNC_LEAVE(tmp);
  return tmp;
}
