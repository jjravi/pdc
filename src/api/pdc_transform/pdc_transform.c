#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <mercury.h>
#include "pdc_server_transform.h"
#include "rpc_engine.h"

#include <pthread.h>
#include <unistd.h>
#include "pdc_private.h"
#include "pdc_transform.h"

// This is my client program that issues 4 concurrent RPCs, each of which includes a bulk transfer driven by the server.

// This code is callback driven (one callback per rpc in this case).
// The callback model could be avoided using the hg_request API which provides a mechanism to wait for completion of an RPC or a subset of RPCs.  This approach would have two drawbacks, however:
// - would require a dedicated thread per concurrent RPC
// - unclear how it would integrate with server-side activity if it were used in that scenario (for server-to-server communication)

extern const char *server_address_str_g;
extern hg_id_t my_rpc_id_g;

static int my_rpc_done = 0;
static pthread_cond_t done_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t done_mutex = PTHREAD_MUTEX_INITIALIZER;
static int req_num = 0;
// static hg_id_t my_rpc_id;

/// struct used to carry state of overall operation across callbacks
struct my_rpc_state_client {
  int value;
  hg_size_t size;
  void *buffer;
  hg_bulk_t bulk_handle;
  hg_handle_t handle;
};

/// callback triggered upon receipt of rpc response
static hg_return_t my_rpc_cb(const struct hg_cb_info *info)
{
  FUNC_ENTER(NULL);
  sleep(5);
  my_rpc_out_t out;
  struct my_rpc_state_client *my_rpc_state_p = info->arg;

  assert(info->ret == HG_SUCCESS);

  // decode response
  int ret = HG_Get_output(info->info.forward.handle, &out);
  assert(ret == 0);

  printf("Got response ret: %d\n", out.ret);

  // clean up resources consumed by this rpc
  HG_Bulk_free(my_rpc_state_p->bulk_handle);
  HG_Free_output(info->info.forward.handle, &out);
  HG_Destroy(info->info.forward.handle);
  free(my_rpc_state_p->buffer);
  free(my_rpc_state_p);

  // signal to main() that we are done
  pthread_mutex_lock(&done_mutex);
  my_rpc_done++;
  pthread_cond_signal(&done_cond);
  pthread_mutex_unlock(&done_mutex);

  FUNC_LEAVE(HG_SUCCESS);
  return HG_SUCCESS;
}

void run_my_rpc(int value)
{
  FUNC_ENTER(NULL);
  hg_addr_t svr_addr;
  // printf("look up: %s\n", server_address_str_g);
  hg_engine_addr_lookup(server_address_str_g, &svr_addr);

  // set up state structure
  struct my_rpc_state_client *my_rpc_state_p = malloc(sizeof(*my_rpc_state_p));
  my_rpc_state_p->size = 512;
  // This includes allocating a src buffer for bulk transfer
  my_rpc_state_p->buffer = malloc(512);
  assert(my_rpc_state_p->buffer);
  sprintf((char *)my_rpc_state_p->buffer, "Hello world!\n");
  my_rpc_state_p->value = value;

  // create create handle to represent this rpc operation
  hg_engine_create_handle(svr_addr, my_rpc_id_g, &my_rpc_state_p->handle);

  // register buffer for rdma/bulk access by server
  const struct hg_info *hgi = HG_Get_info(my_rpc_state_p->handle);
  assert(hgi);
  my_rpc_in_t in;
  int ret = HG_Bulk_create(hgi->hg_class, 1, &my_rpc_state_p->buffer, &my_rpc_state_p->size,
    HG_BULK_READ_ONLY, &in.bulk_handle);
  my_rpc_state_p->bulk_handle = in.bulk_handle;
  assert(ret == 0);

  // Send rpc. Note that we are also transmitting the bulk handle in the input struct.
  in.input_val = my_rpc_state_p->value;
  ret = HG_Forward(my_rpc_state_p->handle, my_rpc_cb, my_rpc_state_p, &in);
  assert(ret == 0);

  hg_engine_addr_free(svr_addr);
  req_num++;
  FUNC_LEAVE(NULL);
}

void wait_my_rpc()
{
  FUNC_ENTER(NULL);
  pthread_mutex_lock(&done_mutex);
  // TODO: need a better way to check complete
  while (my_rpc_done < req_num) pthread_cond_wait(&done_cond, &done_mutex);
  pthread_mutex_unlock(&done_mutex);
  FUNC_LEAVE(NULL);
}

