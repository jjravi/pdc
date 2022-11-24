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

extern const char *server_address_str_g;
extern hg_id_t pdc_transform_id_g;
static pthread_cond_t done_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t done_mutex = PTHREAD_MUTEX_INITIALIZER;
static char req_num = 0;

static void wait_transform_rpcs()
{
  FUNC_ENTER(NULL);
  pthread_mutex_lock(&done_mutex);
  while(req_num) pthread_cond_wait(&done_cond, &done_mutex);
  pthread_mutex_unlock(&done_mutex);
  FUNC_LEAVE(NULL);
}

struct pdc_transform_state_client {
  int value;
  hg_size_t size;
  void *buffer;
  hg_bulk_t bulk_handle;
  hg_handle_t handle;
};

/// callback triggered upon receipt of rpc response
static hg_return_t pdc_transform_cb(const struct hg_cb_info *info)
{
  FUNC_ENTER(NULL);
  pdc_transform_out_t out;
  struct pdc_transform_state_client *pdc_transform_state_p = info->arg;
  assert(info->ret == HG_SUCCESS);

  // decode response
  int ret = HG_Get_output(info->info.forward.handle, &out);
  assert(ret == 0);

  printf("Got response ret: %d\n", out.ret);
  printf("server function address: %p\n", (void *)out.ftn_addr);

  // clean up resources consumed by this rpc
  HG_Bulk_free(pdc_transform_state_p->bulk_handle);
  HG_Free_output(info->info.forward.handle, &out);
  HG_Destroy(info->info.forward.handle);
  // NOTE: Freeing of user buffer happens at application level.
  free(pdc_transform_state_p->buffer);
  free(pdc_transform_state_p);

  // signal to main() that we are done
  pthread_mutex_lock(&done_mutex);
  req_num--;
  pthread_cond_signal(&done_cond);
  pthread_mutex_unlock(&done_mutex);

  FUNC_LEAVE(HG_SUCCESS);
  return HG_SUCCESS;
}

#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
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

#include "pdc_transform.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <mercury.h>
#include "pdc_server_transform.h"
#include "rpc_engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static char *default_pdc_transforms_lib = "libpdctransforms.so";

perr_t pdcTransformRegionRegister(char *func, pdcid_t region_id)
{
  FUNC_ENTER(NULL);
  printf("registering function: %s\n", func);

  // client ties function address to region
  // region will 

  perr_t ret_value = SUCCEED;

  // 1. read file
  char *dir_path = NULL;
  char *userdefinedftn = strdup(func);
  char *transformslibrary = default_pdc_transforms_lib;
  char *colonsep = NULL;
  if ((colonsep = strrchr(userdefinedftn, ':')) != NULL)
  {
    *colonsep++ = 0;
    char *relpath = colonsep;
    transformslibrary = basename(relpath);
    dir_path = dirname(relpath);
  }
  else
  {
    char *thisApp = PDC_get_argv0_();
    dir_path = dirname(strdup(thisApp));
  }
  char *loadpath = PDC_get_realpath(transformslibrary, dir_path);

  size_t func_name_length = strlen(userdefinedftn);
  size_t file_name_length = strlen(transformslibrary);
  void *file_buffer;
  size_t file_size;
  {
    struct stat st;

    if ( stat(loadpath, &st) < 0 ) {
      error("failed to stat");
    }

    // printf("lib size is %zu\n", st.st_size);
    file_size = st.st_size;
    file_buffer = malloc( st.st_size );
    FILE *file = fopen(loadpath, "r");

    size_t read = fread(file_buffer, 1, st.st_size, file); 
    // printf("read %zu bytes\n", read);

    fclose(file);
  }
  size_t total_payload_size = sizeof(struct factory_payload) + file_size;
  void *opaque_ptr = malloc(total_payload_size);
  printf("opaque_ptr size: %ld\n", total_payload_size);
  // struct factory_payload *payload = (struct factory_payload*)malloc(total_payload_size);
  struct factory_payload *payload = (struct factory_payload*)opaque_ptr;
  payload->func_name_length = func_name_length;
  payload->file_name_length = file_name_length;
  payload->code_size = file_size;
  strncpy(payload->func_name, userdefinedftn, func_name_length);
  strncpy(payload->file_name, transformslibrary, file_name_length);
  // memcpy(payload->code_binary, file_buffer, file_size);
  memcpy(opaque_ptr+sizeof(size_t)*3+NAME_MAX*2, file_buffer, file_size);
  printf("payload->func_name: %s\n", payload->func_name);
  printf("payload->func_name_length: %ld\n", payload->func_name_length);
  printf("payload->file_name: %s\n", payload->file_name);
  printf("payload->file_name_length: %ld\n", payload->file_name_length);
  ////////////////////////////

  // 2. send rpc file to server
  hg_addr_t svr_addr;
  hg_engine_addr_lookup(server_address_str_g, &svr_addr);

  // set up state structure
  struct pdc_transform_state_client *pdc_transform_state_p = malloc(sizeof(*pdc_transform_state_p));

  pdc_transform_state_p->value = 42;
  pdc_transform_state_p->size = total_payload_size;
  pdc_transform_state_p->buffer = (void *)payload;

  // create create handle to represent this rpc operation
  hg_engine_create_handle(svr_addr, pdc_transform_id_g, &pdc_transform_state_p->handle);

   // register buffer for rdma/bulk access by server
   const struct hg_info *hgi = HG_Get_info(pdc_transform_state_p->handle);
   assert(hgi);
   pdc_transform_in_t in;
   int ret = HG_Bulk_create(
     hgi->hg_class, 1, 
     &pdc_transform_state_p->buffer, 
     &pdc_transform_state_p->size,
     HG_BULK_READ_ONLY, &in.bulk_handle);
   pdc_transform_state_p->bulk_handle = in.bulk_handle;
   assert(ret == 0);
 
   // Send rpc. Note that we are also transmitting the bulk handle in the input struct.
   // send transfer_request metadata
   in.obj_id = 0;
   in.buf_size = total_payload_size;
 
   ret = HG_Forward(pdc_transform_state_p->handle, pdc_transform_cb, pdc_transform_state_p, &in);
   assert(ret == 0);
 
   hg_engine_addr_free(svr_addr);
   req_num++;
   wait_transform_rpcs();
   FUNC_LEAVE(ret_value);
}

perr_t pdcTransformObjectRegister(char *func, pdcid_t object_id)
{
  // read file

  // send rpc file to server

  // > server callback dlopen and dlsym

  // > returns function address to client.
}



