/**
 * @file
 * @author John J. Ravi (jjravi)
 *
 */

#include "rpc_engine.h"
#include "pdc_private.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define HG_API_CALL(apiFuncCall)                                         \
{                                                                        \
  hg_return_t _status = apiFuncCall;                                     \
  if (_status != HG_SUCCESS) {                                           \
    fprintf(stderr, "%s:%d: error: function %s failed with error %s.\n", \
      __FILE__, __LINE__, #apiFuncCall, HG_Error_to_string(_status));    \
    exit(-1);                                                            \
  }                                                                      \
}

static hg_context_t *hg_context = NULL;
static hg_class_t *hg_class = NULL;

static pthread_t hg_progress_tid;
static int hg_progress_shutdown_flag = 0;

/// @brief dedicated thread function to drive Mercury progress
static void *hg_progress_fn(void *args)
{
  FUNC_ENTER(NULL);
  (void)args;

  hg_return_t ret;
  unsigned int actual_count;

  while (!hg_progress_shutdown_flag)
  {
    do {
      ret = HG_Trigger(hg_context, 0, 1, &actual_count);
    } while ((ret == HG_SUCCESS) && actual_count && !hg_progress_shutdown_flag);

    if (!hg_progress_shutdown_flag)  HG_Progress(hg_context, 100);
  }

  FUNC_LEAVE(NULL);
  return NULL;
}

/// @brief init and finalize() manage a dedicated thread that will drive all HG progress
void hg_engine_init(hg_bool_t listen, const char *local_addr)
{
  FUNC_ENTER(NULL);
  HG_Set_log_level("warning");

  // boilerplate HG initialization steps
  hg_class = HG_Init(local_addr, listen);
  assert(hg_class);

  hg_context = HG_Context_create(hg_class);
  assert(hg_context);

  // start up thread to drive progress
  int ret = pthread_create(&hg_progress_tid, NULL, hg_progress_fn, NULL);
  assert(ret == 0);
  FUNC_LEAVE(NULL);
}

/// @brief init and finalize() manage a dedicated thread that will drive all HG progress
void hg_engine_finalize()
{
  FUNC_ENTER(NULL);
  // tell progress thread to wrap things up
  hg_progress_shutdown_flag = 1;

  // wait for it to shutdown cleanly
  int ret = pthread_join(hg_progress_tid, NULL);
  assert(ret == 0);

  // clean up
  HG_API_CALL(HG_Context_destroy(hg_context));
  HG_API_CALL(HG_Finalize(hg_class));
  FUNC_LEAVE(NULL);
}

hg_class_t *hg_engine_get_class(void)
{
  FUNC_ENTER(NULL);
  FUNC_LEAVE(hg_class);
  return (hg_class);
}

void hg_engine_save_self_addr(void)
{
  FUNC_ENTER(NULL);
  hg_addr_t addr;
  char buf[64] = {'\0'};
  hg_size_t buf_size = 64;

  HG_API_CALL(HG_Addr_self(hg_class, &addr));
  HG_API_CALL(HG_Addr_to_string(hg_class, buf, &buf_size, addr));

  printf("server address string: \"%s\"\n", buf);

  // server writes a port.cfg
  //////////////////////////////
  FILE *fp = fopen( "port.cfg" , "w" );
  fwrite(buf , 1 , buf_size, fp);
  fclose(fp);
  //////////////////////////////

  HG_API_CALL(HG_Addr_free(hg_class, addr));
  FUNC_LEAVE(NULL);
}

void hg_engine_addr_lookup(const char *name, hg_addr_t *addr)
{
  FUNC_ENTER(NULL);
  HG_API_CALL(HG_Addr_lookup2(hg_class, name, addr));
  FUNC_LEAVE(NULL);
}

void hg_engine_addr_free(hg_addr_t addr)
{
  FUNC_ENTER(NULL);
  HG_API_CALL(HG_Addr_free(hg_class, addr));
  FUNC_LEAVE(NULL);
}

void hg_engine_create_handle(hg_addr_t addr, hg_id_t id, hg_handle_t *handle)
{
  FUNC_ENTER(NULL);
  HG_API_CALL(HG_Create(hg_context, addr, id, handle));
  FUNC_LEAVE(NULL);
}

