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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <mercury.h>
#include "pdc_server_persist.h"
#include "rpc_engine.h"

#include "hash-table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "hash-int.h"
#include "compare-int.h"
#include "hash-string.h"
#include "compare-string.h"

#define NUM_TEST_VALUES 10

int value1 = 1, value2 = 2, value3 = 3, value4 = 4;
int allocated_keys = 0;
int allocated_values = 0;

#include <pthread.h>
#include <unistd.h>
#include "pdc_private.h"

static HashTable *hash_table = NULL;

// TODO: jjravi initialize elsewhere
// static unsigned int g_seed;
static unsigned int g_seed = 42;

// Used to seed the generator.           
static void fast_srand(int seed) {
  g_seed = seed;
}

// Compute a pseudorandom integer.
// Output value in range [0, 32767]
static int fast_rand() {
  g_seed = (214013*g_seed+2531011);
  return (g_seed>>16)&0x7FFF;
}

// Create a new key
uint64_t *new_key(uint64_t value)
{
  uint64_t *result;
  result = malloc(sizeof(uint64_t));
  *result = value;
  ++allocated_keys;
  return result;
}

//Callback function invoked when a key is freed
void free_key(void *key)
{
  free(key);
  --allocated_keys;
}

// Create a new value
bool *new_value(bool value)
{
  bool *result;
  result = malloc(sizeof(bool));
  *result = value;
  ++allocated_values;
  return result;
}

// Callback function invoked when a value is freed
void free_value(void *value)
{
  free(value);
  --allocated_values;
}

/* Generates a hash table for use in tests containing 10,000 entries */

HashTable *generate_hash_table(void)
{
  HashTable *hash_table;
  char buf[10];
  char *value;
  int i;

  /* Allocate a new hash table.  We use a hash table with keys that are
   * string versions of the integer values 0..9999 to ensure that there
   * will be collisions within the hash table (using integer values
   * with int_hash causes no collisions) */

  hash_table = hash_table_new(string_hash, string_equal);

  /* Insert lots of values */

  for (i=0; i<NUM_TEST_VALUES; ++i) {
    sprintf(buf, "%i", i);

    value = strdup(buf);

    hash_table_insert(hash_table, value, value);
  }

  /* Automatically free all the values with the hash table */

  hash_table_register_free_functions(hash_table, NULL, free);

  return hash_table;
}

/* Basic allocate and free */

void test_hash_table_new_free(void)
{
  HashTable *hash_table;

  hash_table = hash_table_new(int_hash, int_equal);

  assert(hash_table != NULL);

  /* Add some values */

  hash_table_insert(hash_table, &value1, &value1);
  hash_table_insert(hash_table, &value2, &value2);
  hash_table_insert(hash_table, &value3, &value3);
  hash_table_insert(hash_table, &value4, &value4);

  /* Free the hash table */

  hash_table_free(hash_table);
}

/* Test insert and lookup functions */

void test_hash_table_insert_lookup(void)
{
  HashTable *hash_table;
  char buf[10];
  char *value;
  int i;

  /* Generate a hash table */

  hash_table = generate_hash_table();

  assert(hash_table_num_entries(hash_table) == NUM_TEST_VALUES);

  /* Check all values */

  for (i=0; i<NUM_TEST_VALUES; ++i) {
    sprintf(buf, "%i", i);
    value = hash_table_lookup(hash_table, buf);

    assert(strcmp(value, buf) == 0);
  }

  /* Lookup on invalid values returns NULL */

  sprintf(buf, "%i", -1);
  assert(hash_table_lookup(hash_table, buf) == NULL);
  sprintf(buf, "%i", NUM_TEST_VALUES);
  assert(hash_table_lookup(hash_table, buf) == NULL);

  /* Insert overwrites existing entries with the same key */

  sprintf(buf, "%i", 12345);
  hash_table_insert(hash_table, buf, strdup("hello world"));
  value = hash_table_lookup(hash_table, buf);
  assert(strcmp(value, "hello world") == 0);

  hash_table_free(hash_table);
}

void test_hash_table_remove(void)
{
  HashTable *hash_table;
  char buf[10];

  hash_table = generate_hash_table();

  assert(hash_table_num_entries(hash_table) == NUM_TEST_VALUES);
  sprintf(buf, "%i", 5000);
  assert(hash_table_lookup(hash_table, buf) != NULL);

  /* Remove an entry */

  hash_table_remove(hash_table, buf);

  /* Check entry counter */

  assert(hash_table_num_entries(hash_table) == 9999);

  /* Check that NULL is returned now */

  assert(hash_table_lookup(hash_table, buf) == NULL);

  /* Try removing a non-existent entry */

  sprintf(buf, "%i", -1);
  hash_table_remove(hash_table, buf);

  assert(hash_table_num_entries(hash_table) == 9999);

  hash_table_free(hash_table);
}

void test_hash_table_iterating(void)
{
  HashTable *hash_table;
  HashTableIterator iterator;
  int count;

  hash_table = generate_hash_table();

  /* Iterate over all values in the table */

  count = 0;

  hash_table_iterate(hash_table, &iterator);

  while (hash_table_iter_has_more(&iterator)) {
    hash_table_iter_next(&iterator);

    ++count;
  }

  assert(count == NUM_TEST_VALUES);

  /* Test iter_next after iteration has completed. */

  HashTablePair pair = hash_table_iter_next(&iterator);
  assert(pair.value == HASH_TABLE_NULL);

  hash_table_free(hash_table);

  /* Test iterating over an empty table */

  hash_table = hash_table_new(int_hash, int_equal);

  hash_table_iterate(hash_table, &iterator);

  assert(hash_table_iter_has_more(&iterator) == 0);

  hash_table_free(hash_table);
}

/* Demonstrates the ability to iteratively remove objects from
 * a hash table: ie. removing the current key being iterated over
 * does not break the iterator. */
void test_hash_table_iterating_remove(void)
{
  HashTable *hash_table;
  HashTableIterator iterator;
  char buf[10];
  char *val;
  HashTablePair pair;
  int count;
  unsigned int removed;
  int i;

  hash_table = generate_hash_table();

  /* Iterate over all values in the table */

  count = 0;
  removed = 0;

  hash_table_iterate(hash_table, &iterator);

  while (hash_table_iter_has_more(&iterator)) {

    /* Read the next value */

    pair = hash_table_iter_next(&iterator);
    val = pair.value;

    /* Remove every hundredth entry */

    if ((atoi(val) % 100) == 0) {
      hash_table_remove(hash_table, val);
      ++removed;
    }

    ++count;
  }

  /* Check counts */

  assert(removed == 100);
  assert(count == NUM_TEST_VALUES);

  assert(hash_table_num_entries(hash_table)
    == NUM_TEST_VALUES - removed);

  /* Check all entries divisible by 100 were really removed */

  for (i=0; i<NUM_TEST_VALUES; ++i) {
    sprintf(buf, "%i", i);

    if (i % 100 == 0) {
      assert(hash_table_lookup(hash_table, buf) == NULL);
    } else {
      assert(hash_table_lookup(hash_table, buf) != NULL);
    }
  }

  hash_table_free(hash_table);
}



extern const char *server_address_str_g;
extern hg_id_t pdc_persist_id_g;

static int pdc_persist_done = 0;
static pthread_cond_t done_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t done_mutex = PTHREAD_MUTEX_INITIALIZER;
static int req_num = 0;
// static hg_id_t pdc_persist_id;

/// struct used to carry state of overall operation across callbacks
struct pdc_persist_state_client {
  uint64_t uid;
  hg_size_t size;
  void *buffer;
  hg_bulk_t bulk_handle;
  hg_handle_t handle;
};

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

/// callback triggered upon receipt of rpc response
static hg_return_t pdc_persist_cb(const struct hg_cb_info *info)
{
  FUNC_ENTER(NULL);
  pdc_persist_out_t out;
  struct pdc_persist_state_client *pdc_persist_state_p = info->arg;

  /////////////////////////////////
  {
    uint64_t key = pdc_persist_state_p->uid;
    assert(hash_table_lookup(hash_table, &key) != NULL);

    hash_table_remove(hash_table, &key);
  }
  /////////////////////////////////

  assert(info->ret == HG_SUCCESS);

  // decode response
  int ret = HG_Get_output(info->info.forward.handle, &out);
  assert(ret == 0);

  printf("Got response ret: %d\n", out.ret);

  // clean up resources consumed by this rpc
  HG_Bulk_free(pdc_persist_state_p->bulk_handle);
  HG_Free_output(info->info.forward.handle, &out);
  HG_Destroy(info->info.forward.handle);
  // NOTE: Freeing of user buffer happens at application level.
  // free(pdc_persist_state_p->buffer);
  free(pdc_persist_state_p);

  // signal to main() that we are done
  pthread_mutex_lock(&done_mutex);
  pdc_persist_done++;
  pthread_cond_signal(&done_cond);
  pthread_mutex_unlock(&done_mutex);

  FUNC_LEAVE(HG_SUCCESS);
  return HG_SUCCESS;
}

void run_pdc_persist(pdc_transfer_request *trans_request, pdcid_t transfer_id)
{
  FUNC_ENTER(NULL);
  hg_addr_t svr_addr;
  // printf("look up: %s\n", server_address_str_g);
  hg_engine_addr_lookup(server_address_str_g, &svr_addr);

  // set up state structure
  struct pdc_persist_state_client *pdc_persist_state_p = malloc(sizeof(*pdc_persist_state_p));

  {
    uint64_t *key = new_key(transfer_id);
    bool *value = new_value(false);
    hash_table_insert(hash_table, key, value);
    pdc_persist_state_p->uid = *key;
  }

  assert(trans_request->buf);
  assert(trans_request->total_data_size != 0);

  pdc_persist_state_p->size = trans_request->total_data_size;
  pdc_persist_state_p->buffer = trans_request->buf;

  // create create handle to represent this rpc operation
  hg_engine_create_handle(svr_addr, pdc_persist_id_g, &pdc_persist_state_p->handle);

  // register buffer for rdma/bulk access by server
  const struct hg_info *hgi = HG_Get_info(pdc_persist_state_p->handle);
  assert(hgi);
  pdc_persist_in_t in;
  int ret = HG_Bulk_create(
    hgi->hg_class, 1, 
    &pdc_persist_state_p->buffer, 
    &pdc_persist_state_p->size,
    HG_BULK_READ_ONLY, &in.bulk_handle);
  pdc_persist_state_p->bulk_handle = in.bulk_handle;
  assert(ret == 0);

  // Send rpc. Note that we are also transmitting the bulk handle in the input struct.
  // send transfer_request metadata
  in.buf_size = trans_request->total_data_size;
  pack_region_metadata(trans_request->remote_region_ndim, trans_request->remote_region_offset, trans_request->remote_region_size, &(in.remote_region));
  // in.remote_region
  in.obj_id = trans_request->obj_id;
  in.obj_ndim = trans_request->obj_ndim;
  switch (trans_request->obj_ndim)
  {
    case 3:
      in.obj_dim2 = trans_request->obj_dims[2];
    case 2:
      in.obj_dim1 = trans_request->obj_dims[1];
    case 1:
      in.obj_dim0 = trans_request->obj_dims[0];
      break;
  }
  in.remote_unit = trans_request->unit;
  in.meta_server_id = trans_request->metadata_server_id;
  in.access_type = trans_request->access_type;

  ///////////////////////////////////////

  ret = HG_Forward(pdc_persist_state_p->handle, pdc_persist_cb, pdc_persist_state_p, &in);
  assert(ret == 0);

  hg_engine_addr_free(svr_addr);
  req_num++;
  FUNC_LEAVE(NULL);
}

perr_t pdcTransferCreate(pdcid_t *transfer_id, pdcid_t *legacy_id, void *buf, pdc_access_t access_type, pdcid_t obj_id, pdcid_t local_reg, pdcid_t remote_reg)
{
    pdcid_t                 ret_value = SUCCEED;
    struct _pdc_id_info *   objinfo2;
    struct _pdc_obj_info *  obj2;
    pdc_transfer_request *  p;
    struct _pdc_id_info *   reginfo1, *reginfo2;
    struct pdc_region_info *reg1, *reg2;
    uint64_t *              ptr;
    uint64_t                unit;
    int                     j;

    FUNC_ENTER(NULL);
    reginfo1 = PDC_find_id(local_reg);
    reg1     = (struct pdc_region_info *)(reginfo1->obj_ptr);
    reginfo2 = PDC_find_id(remote_reg);
    reg2     = (struct pdc_region_info *)(reginfo2->obj_ptr);
    objinfo2 = PDC_find_id(obj_id);
    if (objinfo2 == NULL)
        PGOTO_ERROR(FAIL, "cannot locate remote object ID");
    obj2 = (struct _pdc_obj_info *)(objinfo2->obj_ptr);
    // remote_meta_id = obj2->obj_info_pub->meta_id;

    p                   = PDC_MALLOC(pdc_transfer_request);
    p->obj_pointer      = obj2;
    p->mem_type         = obj2->obj_pt->obj_prop_pub->type;
    p->obj_id           = obj2->obj_info_pub->meta_id;
    p->access_type      = access_type;
    p->buf              = buf;
    p->metadata_id      = NULL;
    p->read_bulk_buf    = NULL;
    p->new_buf          = NULL;
    p->bulk_buf         = NULL;
    p->bulk_buf_ref     = NULL;
    p->output_buf       = NULL;
    p->region_partition = ((pdc_metadata_t *)obj2->metadata)->region_partition;
    // p->region_partition   = PDC_REGION_LOCAL;
    p->data_server_id     = ((pdc_metadata_t *)obj2->metadata)->data_server_id;
    p->metadata_server_id = obj2->obj_info_pub->metadata_server_id;
    p->unit               = PDC_get_var_type_size(p->mem_type);
    p->consistency        = obj2->obj_pt->obj_prop_pub->consistency;
    unit                  = p->unit;

    /*
        printf("creating a request from obj %s metadata id = %llu, access_type = %d\n",
       obj2->obj_info_pub->name, (long long unsigned)obj2->obj_info_pub->meta_id, access_type);
    */
    p->local_region_ndim   = reg1->ndim;
    p->local_region_offset = (uint64_t *)malloc(sizeof(uint64_t) * (reg1->ndim * 2 + reg2->ndim * 2 + obj2->obj_pt->obj_prop_pub->ndim));
    ptr = p->local_region_offset;
    memcpy(p->local_region_offset, reg1->offset, sizeof(uint64_t) * reg1->ndim);
    ptr += reg1->ndim;
    p->local_region_size = ptr;
    memcpy(p->local_region_size, reg1->dims_size, sizeof(uint64_t) * reg1->ndim);
    ptr += reg1->ndim;

    p->remote_region_ndim   = reg2->ndim;
    p->remote_region_offset = ptr;
    memcpy(p->remote_region_offset, reg2->offset, sizeof(uint64_t) * reg2->ndim);
    ptr += reg2->ndim;

    p->remote_region_size = ptr;
    memcpy(p->remote_region_size, reg2->dims_size, sizeof(uint64_t) * reg2->ndim);
    ptr += reg2->ndim;

    p->obj_ndim = obj2->obj_pt->obj_prop_pub->ndim;
    p->obj_dims = ptr;
    memcpy(p->obj_dims, obj2->obj_pt->obj_prop_pub->dims, sizeof(uint64_t) * p->obj_ndim);

    p->total_data_size = unit;
    for (j = 0; j < (int)reg2->ndim; ++j) {
        p->total_data_size *= reg2->dims_size[j];
    }
    /*
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            printf("rank = %d transfer request create check obj ndim %d, dims [%lld, %lld, %lld],
       local_offset[0] = %lld, " "reg1->offset[0] = %lld\n", rank, (int)p->obj_ndim, (long long
       int)p->obj_dims[0], (long long int)p->obj_dims[1], (long long int)p->obj_dims[2], (long long
       int)p->local_region_offset[0], (long long int)reg1->offset[0]);
    */
    *legacy_id = PDC_id_register(PDC_TRANSFER_REQUEST, p);
    *transfer_id = fast_rand();
done:
    FUNC_LEAVE(ret_value);
}

perr_t pdcTransferStart(pdcid_t transfer_request_id, pdcid_t transfer_id)
{
  perr_t                ret_value = SUCCEED;
  struct _pdc_id_info * transferinfo;
  pdc_transfer_request *transfer_request;
  size_t                unit;
  int                   i;

  FUNC_ENTER(NULL);

  if (!hash_table) {
    char buf[10];
    hash_table = generate_hash_table();
    for(int ii = 0; ii < NUM_TEST_VALUES; ii++)
    {
      sprintf(buf, "%i", ii);
      hash_table_remove(hash_table, buf);
    }
  }

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

    /*
    ret_value = PDC_Client_transfer_request(
      transfer_request->new_buf, transfer_request->obj_id, transfer_request->data_server_id,
      transfer_request->obj_ndim, transfer_request->obj_dims, transfer_request->remote_region_ndim,
      transfer_request->remote_region_offset, transfer_request->remote_region_size, unit,
      transfer_request->access_type, transfer_request->metadata_id);
      */
  }

  run_pdc_persist(transfer_request, transfer_id);
done:
  FUNC_LEAVE(ret_value);
}

perr_t pdcTransferWait(pdcid_t transfer_id)
{
  FUNC_ENTER(NULL);
  while(hash_table_lookup(hash_table, &transfer_id) != NULL)
  {
    // TODO: jjravi busy wait can be exponential backup instead to save CPU cycles
    usleep(10000);
  }
  FUNC_LEAVE(NULL);
  return SUCCEED;
}

void wait_pdc_persist()
{
  FUNC_ENTER(NULL);
  pthread_mutex_lock(&done_mutex);
  // TODO: need a better way to check complete
  while (pdc_persist_done < req_num) pthread_cond_wait(&done_cond, &done_mutex);
  pthread_mutex_unlock(&done_mutex);
  FUNC_LEAVE(NULL);
}


