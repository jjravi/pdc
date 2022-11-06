#include <aio.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "pdc_server_persist.h"
#include "pdc_private.h"

#define HG_API_CALL(apiFuncCall)                                         \
{                                                                        \
  hg_return_t _status = apiFuncCall;                                     \
  if (_status != HG_SUCCESS) {                                           \
    fprintf(stderr, "%s:%d: error: function %s failed with error %s.\n", \
      __FILE__, __LINE__, #apiFuncCall, HG_Error_to_string(_status));    \
    exit(-1);                                                            \
  }                                                                      \
}

// pdc_persist: This is a RPC operation. It includes a small bulk transfer, driven by the server, that moves data from the client to the server.  The server writes the data to a local file.

// There are 3 key callbacks here:
// - pdc_persist_handler(): handles an incoming RPC operation
// - pdc_persist_handler_bulk_cb(): handles completion of bulk transfer
// - pdc_persist_handler_write_cb(): handles completion of async write and sends response


// NOTES: this is all event-driven. Data is written using an aio operation with SIGEV_THREAD notification.
//
// Note that the open and close are blocking for now because there is no standard aio variant of those functions.
//
// All I/O calls *could* be blocking here. The problem with that approach is that you would need to use a thread pool with mercury to prevent it from stalling while running callbacks, and the threadpool size would then dictate both request concurrency and I/O concurrency simultaneously.  You would still also need to handle callbacks for the HG transfer.

/// struct used to carry state of overall operation across callbacks
struct pdc_persist_state {
  hg_size_t size;
  void *data_buffer;
  hg_bulk_t bulk_handle;
  hg_handle_t handle;
  // struct aiocb acb;
  pdc_persist_in_t in;
};

/// callback triggered upon completion of async write
static void pdc_persist_handler_write_cb(union sigval sig)
{
  FUNC_ENTER(NULL);
  struct pdc_persist_state *pdc_persist_state_p = sig.sival_ptr;
  // int ret = aio_error(&pdc_persist_state_p->acb);
  // assert(ret == 0);

  pdc_persist_out_t out;
  out.ret = 0;

  // NOTE: really this should be nonblocking
  // close(pdc_persist_state_p->acb.aio_fildes);

  // send ack to client
  // NOTE: don't bother specifying a callback here for completion of sending response. This is just a best effort response.
  HG_API_CALL(HG_Respond(pdc_persist_state_p->handle, NULL, NULL, &out));

  HG_Bulk_free(pdc_persist_state_p->bulk_handle);
  HG_Destroy(pdc_persist_state_p->handle);
  free(pdc_persist_state_p->data_buffer);
  free(pdc_persist_state_p);

  FUNC_LEAVE(NULL);
  return;
}

/// callback triggered upon completion of bulk transfer
static hg_return_t pdc_persist_handler_bulk_cb(const struct hg_cb_info *info)
{
  FUNC_ENTER(NULL);
  assert(info->ret == 0);

  // TODO: should call ~/hpc-io/pdc/src/server/pdc_client_server_common.c:343
  // transfer_request_bulk_transfer_write_cb()

  struct pdc_persist_state *pdc_persist_state_p = info->arg;

  /////////////////////////////////////////
  {
    struct pdc_region_info *remote_reg_info;
    uint64_t obj_dims[3];
    remote_reg_info = (struct pdc_region_info *)malloc(sizeof(struct pdc_region_info));
    remote_reg_info->ndim   = (pdc_persist_state_p->in.remote_region).ndim;
    remote_reg_info->offset = (uint64_t *)malloc(remote_reg_info->ndim * sizeof(uint64_t));
    remote_reg_info->dims_size   = (uint64_t *)malloc(remote_reg_info->ndim * sizeof(uint64_t));

    // TODO: jjravi replace with for loop
    if (remote_reg_info->ndim >= 1) {
      (remote_reg_info->offset)[0] = (pdc_persist_state_p->in.remote_region).start_0;
      (remote_reg_info->dims_size)[0]   = (pdc_persist_state_p->in.remote_region).count_0;
      obj_dims[0]                  = (pdc_persist_state_p->in).obj_dim0;
    }
    if (remote_reg_info->ndim >= 2) {
      (remote_reg_info->offset)[1] = (pdc_persist_state_p->in.remote_region).start_1;
      (remote_reg_info->dims_size)[1]   = (pdc_persist_state_p->in.remote_region).count_1;
      obj_dims[1]                  = (pdc_persist_state_p->in).obj_dim1;
    }
    if (remote_reg_info->ndim >= 3) {
      (remote_reg_info->offset)[2] = (pdc_persist_state_p->in.remote_region).start_2;
      (remote_reg_info->dims_size)[2]   = (pdc_persist_state_p->in.remote_region).count_2;
      obj_dims[2]                  = (pdc_persist_state_p->in).obj_dim2;
    }
    /////////
    remote_reg_info->unit = pdc_persist_state_p->in.remote_unit;
    remote_reg_info->data_buf = pdc_persist_state_p->data_buffer;

    // TODO: region size is different from bytes (compression)
    remote_reg_info->data_size = pdc_persist_state_p->in.buf_size;
    // remote_reg_info->data_size = remote_reg_info->unit * remote_reg_info->dims_size[0];
    for(size_t i = 1; i < remote_reg_info->ndim; i++)
    {
      remote_reg_info->data_size *= remote_reg_info->dims_size[i];
    }

#ifdef PDC_SERVER_CACHE
    PDC_transfer_request_data_write_out(
      pdc_persist_state_p->in.obj_id,
      pdc_persist_state_p->in.obj_ndim,
      obj_dims,
      remote_reg_info
    );
#else
    PDC_Server_transfer_request_io(
      pdc_persist_state_p->in.obj_id,
      pdc_persist_state_p->in.obj_ndim,
      obj_dims,
      remote_reg_info, 1
    );
#endif

//    pthread_mutex_lock(&transfer_request_status_mutex);
//    PDC_finish_request(local_bulk_args->transfer_request_id);
//    pthread_mutex_unlock(&transfer_request_status_mutex);
//    free(remote_reg_info->data_buf);
//    free(remote_reg_info);
//
//    HG_Bulk_free(local_bulk_args->bulk_handle);
  }
  /////////////////////////////////////////

  // TODO: jjravi use aio instead of posix for persisted storage 
  /*
  {
    // open file (NOTE: this is blocking for now, for simplicity )
    char filename[256];
    static int input_val = 0;
    input_val++;
    sprintf(filename, "/tmp/hg-stock-%d.txt", input_val);
    memset(&pdc_persist_state_p->acb, 0, sizeof(pdc_persist_state_p->acb));
    pdc_persist_state_p->acb.aio_fildes = open(filename, O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR);
    assert(pdc_persist_state_p->acb.aio_fildes > -1);

    // set up async I/O operation (write the bulk data that we just pulled from the client)
    pdc_persist_state_p->acb.aio_offset = 0;
    pdc_persist_state_p->acb.aio_buf = pdc_persist_state_p->data_buffer;
    pdc_persist_state_p->acb.aio_nbytes = 512;
    pdc_persist_state_p->acb.aio_sigevent.sigev_notify = SIGEV_THREAD;
    pdc_persist_state_p->acb.aio_sigevent.sigev_notify_attributes = NULL;
    pdc_persist_state_p->acb.aio_sigevent.sigev_notify_function = pdc_persist_handler_write_cb;
    pdc_persist_state_p->acb.aio_sigevent.sigev_value.sival_ptr = pdc_persist_state_p;

    // post async write (just dump data to stdout)
    int ret = aio_write(&pdc_persist_state_p->acb);
    assert(ret == 0);
  }
  */
  union sigval sig;
  sig.sival_ptr = pdc_persist_state_p;
  pdc_persist_handler_write_cb(sig);

  HG_API_CALL(HG_Bulk_free(pdc_persist_state_p->in.bulk_handle));
  HG_API_CALL(HG_Destroy(pdc_persist_state_p->handle));
  FUNC_LEAVE(0);
  return 0;
}

/// callback/handler triggered upon receipt of rpc request
static hg_return_t pdc_persist_handler(hg_handle_t handle)
{
  // TODO: should do the same as pdc_client_server_common.c:724
  // transfer_request, handle
  FUNC_ENTER(NULL);
  // set up state structure
  struct pdc_persist_state *pdc_persist_state_p = malloc(sizeof(*pdc_persist_state_p));
  assert(pdc_persist_state_p);
  pdc_persist_state_p->size = 512;
  pdc_persist_state_p->handle = handle;

  // decode input
  HG_API_CALL(HG_Get_input(handle, &pdc_persist_state_p->in));

  // This includes allocating a target buffer for bulk transfer
  pdc_persist_state_p->data_buffer = malloc(pdc_persist_state_p->in.buf_size);
  assert(pdc_persist_state_p->data_buffer);

  printf("Got RPC request with buf_size: %ld\n", pdc_persist_state_p->in.buf_size);

  // register local target buffer for bulk access
  const struct hg_info *hgi = HG_Get_info(handle);
  assert(hgi);
  int ret = HG_Bulk_create(hgi->hg_class, 1, &pdc_persist_state_p->data_buffer, &pdc_persist_state_p->size,
    HG_BULK_WRITE_ONLY, &pdc_persist_state_p->bulk_handle);
  assert(ret == 0);

  // initiate bulk transfer from client to server
  ret = HG_Bulk_transfer(hgi->context, pdc_persist_handler_bulk_cb, pdc_persist_state_p,
    HG_BULK_PULL, hgi->addr, pdc_persist_state_p->in.bulk_handle, 0,
    pdc_persist_state_p->bulk_handle, 0, pdc_persist_state_p->size, HG_OP_ID_IGNORE);
  assert(ret == 0);

  FUNC_LEAVE(0);
  return 0;
}

/// register pdc_persist type with Mercury
hg_id_t pdc_persist_register()
{
  FUNC_ENTER(NULL);
  hg_class_t *hg_class = hg_engine_get_class();
  hg_id_t tmp = MERCURY_REGISTER(hg_class, "pdc_persist", pdc_persist_in_t, pdc_persist_out_t, pdc_persist_handler);
  FUNC_LEAVE(tmp);
  return tmp;
}

