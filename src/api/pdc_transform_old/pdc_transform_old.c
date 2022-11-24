/*
 * Copyright Notice for
 * Proactive Data Containers (PDC) Software Library and Utilities
 * -----------------------------------------------------------------------------

 *** Copyright Notice ***

 * Proactive Data Containers (PDC) Copyright (c) 2017, The Regents of the
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
 */

#include "pdc_malloc.h"
#include "pdc_private.h"
#include "pdc_id_pkg.h"
#include "pdc_prop.h"
#include "pdc_obj.h"
#include "pdc_obj_pkg.h"
#include "pdc_region_pkg.h"
#include "pdc_region.h"
#include "pdc_interface.h"
#include "pdc_transform_old.h"
#include "pdc_analysis_pkg.h"
#include "pdc_client_server_common.h"

#include "pdc_client_connect.h"
#include "pdc_utlist.h"
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>

extern int pdc_client_mpi_rank_g;
extern int pdc_client_mpi_size_g;

static char *default_pdc_transforms_lib = "libpdctransforms.so";

typedef struct {
  void * data;
  int size;
  int current;
} lib_t;

lib_t libdata;

static bool load_library_from_file(char * path, lib_t *libdata) {
  struct stat st;
  FILE * file;
  size_t read;

  if ( stat(path, &st) < 0 ) {
    error("failed to stat");
    return false;
  }

  printf("lib size is %zu", st.st_size); 

  libdata->size = st.st_size;
  libdata->data = malloc( st.st_size );
  libdata->current = 0;

  file = fopen(path, "r");

  read = fread(libdata->data, 1, st.st_size, file); 
  printf("read %zu bytes", read);

  fclose(file);

  return true;
}

perr_t
PDCobj_transform_register(char *func, pdcid_t iterIn, pdcid_t iterOut)
{
  perr_t ret_value                              = SUCCEED; /* Return value */
  void * ftnHandle                              = NULL;
  int (*ftnPtr)(pdcid_t, pdcid_t)               = NULL;
  struct _pdc_region_analysis_ftn_info *thisFtn = NULL;
  struct _pdc_iterator_info *           i_in = NULL, *i_out = NULL;
  pdcid_t                               meta_id_in = 0, meta_id_out = 0;
  pdcid_t                               local_id_in = 0, local_id_out = 0;
  char *                                thisApp        = NULL;
  char *                                colonsep       = NULL;
  char *                                analyislibrary = NULL;
  char *                                applicationDir = NULL;
  char *                                userdefinedftn = NULL;
  char *                                loadpath       = NULL;

  FUNC_ENTER(NULL);

  thisApp = PDC_get_argv0_();
  if (thisApp) {
    applicationDir = dirname(strdup(thisApp));
  }
  userdefinedftn = strdup(func);

  if ((colonsep = strrchr(userdefinedftn, ':')) != NULL) {
    *colonsep++    = 0;
    analyislibrary = colonsep;
  }
  else
    analyislibrary = default_pdc_transforms_lib;

  // TODO: Should probably validate the location of the "analysislibrary"
  loadpath = PDC_get_realpath(analyislibrary, applicationDir);
  if (PDC_get_ftnPtr_((const char *)userdefinedftn, (const char *)loadpath, &ftnHandle) < 0)
    PGOTO_ERROR(FAIL, "PDC_get_ftnPtr_ returned an error!");

  if ((ftnPtr = ftnHandle) == NULL)
    PGOTO_ERROR(FAIL, "Analysis function lookup failed");

  if ((thisFtn = PDC_MALLOC(struct _pdc_region_analysis_ftn_info)) == NULL)
    PGOTO_ERROR(FAIL, "PDC register_obj_analysis memory allocation failed");

  thisFtn->ftnPtr = (int (*)())ftnPtr;
  thisFtn->n_args = 2;
  /* Allocate for iterator ids and region ids */
  if ((thisFtn->object_id = (pdcid_t *)calloc(4, sizeof(pdcid_t))) != NULL) {
    thisFtn->object_id[0] = iterIn;
    thisFtn->object_id[1] = iterOut;
  }
  else
    PGOTO_ERROR(FAIL, "PDC register_obj_analysis memory allocation failed - object_ids");

  thisFtn->region_id = (pdcid_t *)&thisFtn->object_id[2];

  thisFtn->lang = C_lang;

  if (PDC_Block_iterator_cache) {
    if (iterIn != 0) {
      i_in        = &PDC_Block_iterator_cache[iterIn];
      meta_id_in  = i_in->meta_id;
      local_id_in = i_in->local_id;
    }
    if (iterOut != 0) {
      i_out        = &PDC_Block_iterator_cache[iterOut];
      meta_id_out  = i_out->meta_id;
      local_id_out = i_out->local_id;
    }
  }

  PDC_Client_register_obj_analysis(thisFtn, userdefinedftn, loadpath, local_id_in, local_id_out, meta_id_in,
    meta_id_out);

  // Add region IDs
  thisFtn->region_id[0] = i_in->reg_id;
  thisFtn->region_id[1] = i_out->reg_id;

  // Add to our own list of analysis functions
  if (PDC_add_analysis_ptr_to_registry_(thisFtn) < 0)
    PGOTO_ERROR(FAIL, "PDC unable to register analysis function!");

done:
  if (applicationDir)
    free(applicationDir);
  if (userdefinedftn)
    free(userdefinedftn);
  if (loadpath)
    free(loadpath);

  FUNC_LEAVE(ret_value);
}



// perr_t
// PDCobj_transform_register(char *func, pdcid_t obj_id, int current_state, int next_state,
//   pdc_obj_transform_t op_type, pdc_data_movement_t when)
// {
//   perr_t ret_value                               = SUCCEED;
//   void * ftnHandle                               = NULL;
//   size_t (*ftnPtr)()                             = NULL;
//   struct _pdc_region_transform_ftn_info *thisFtn = NULL;
//   struct _pdc_obj_info *                 obj1, *obj2;
//   struct _pdc_id_info *                  objinfo1;
//   struct _pdc_obj_prop *                 prop;
//   struct pdc_region_info *               reg1 = NULL, *reg2 = NULL;
//   pdcid_t                                src_region_id = 0, dest_region_id = 0;
//   pdcid_t                                dest_object_id    = 0;
//   char *                                 thisApp           = NULL;
//   char *                                 colonsep          = NULL;
//   char *                                 transformslibrary = NULL;
//   char *                                 applicationDir    = NULL;
//   char *                                 userdefinedftn    = NULL;
//   char *                                 loadpath          = NULL;
//   int                                    local_regIndex;
//   struct _pdc_id_info *                  id_info;
// 
//   FUNC_ENTER(NULL);
// 
//   thisApp = PDC_get_argv0_();
//   if (thisApp)
//   {
//     applicationDir = dirname(strdup(thisApp));
//   }
//   userdefinedftn = strdup(func);
// 
//   if ((colonsep = strrchr(userdefinedftn, ':')) != NULL) {
//     *colonsep++       = 0;
//     transformslibrary = colonsep;
//   }
//   else
//     transformslibrary = default_pdc_transforms_lib;
// 
//   loadpath = PDC_get_realpath(transformslibrary, applicationDir);
// 
//   if (PDC_get_ftnPtr_(userdefinedftn, loadpath, &ftnHandle) < 0)
//     PGOTO_ERROR(FAIL, "PDC_get_ftnPtr_ returned an error!\n");
// 
//   if ((ftnPtr = ftnHandle) == NULL)
//     PGOTO_ERROR(FAIL, "Transforms function lookup failed");
// 
//   if ((thisFtn = PDC_MALLOC(struct _pdc_region_transform_ftn_info)) == NULL)
//     PGOTO_ERROR(FAIL, "PDC register_obj_transforms memory allocation failed");
// 
//   memset(thisFtn, 0, sizeof(struct _pdc_region_transform_ftn_info));
//   thisFtn->ftnPtr    = (size_t(*)())ftnPtr;
//   thisFtn->object_id = obj_id;
//   thisFtn->op_type   = op_type;
//   thisFtn->when      = when;
//   thisFtn->lang      = C_lang;
//   thisFtn->nextState = next_state;
//   thisFtn->dest_type = PDC_UNKNOWN;
// 
//   // Add to our own list of transform functions
//   if ((local_regIndex = PDC_add_transform_ptr_to_registry_(thisFtn)) < 0)
//     PGOTO_ERROR(FAIL, "PDC unable to register transform function!");
// 
//   // Flag the transform as being active on mapping operations
//   if (op_type == PDC_DATA_MAP) {
//     objinfo1 = PDC_find_id(obj_id);
//     if (objinfo1 == NULL)
//       PGOTO_ERROR(FAIL, "cannot locate local object ID");
//     obj1 = (struct _pdc_obj_info *)(objinfo1->obj_ptr);
//     /* See if any mapping operations are defined */
//     if (obj1 && (obj1->region_list_head != NULL)) {
//       id_info        = PDC_find_id(obj1->region_list_head->orig_reg_id);
//       src_region_id  = obj1->region_list_head->orig_reg_id;
//       dest_region_id = obj1->region_list_head->des_reg_id;
//       // mapping is already defined...
//       if (id_info && ((reg1 = (struct pdc_region_info *)id_info->obj_ptr) != NULL)) {
//         thisFtn->src_region = reg1;
//         obj1                = reg1->obj;
// 
//         // Requires that the PDCprop_set_obj_buf function be used...
//         if (obj1 && ((prop = obj1->obj_pt) != NULL)) {
//           thisFtn->data        = prop->buf;
//           thisFtn->type        = prop->obj_prop_pub->type;
//           thisFtn->type_extent = PDC_get_var_type_size(prop->obj_prop_pub->type);
//         }
//       }
//       id_info = PDC_find_id(dest_region_id);
//       if (id_info && ((reg2 = (struct pdc_region_info *)id_info->obj_ptr) != NULL)) {
//         thisFtn->dest_region = reg2;
//         obj2                 = reg2->obj;
//         dest_object_id       = obj2->obj_info_pub->local_id;
//         if (obj2 && ((prop = obj2->obj_pt) != NULL)) {
//           thisFtn->result      = prop->buf;
//           thisFtn->dest_type   = prop->obj_prop_pub->type;
//           thisFtn->dest_extent = PDC_get_var_type_size(prop->obj_prop_pub->type);
// 
//         }
//       }
//       // Flag the destination region with the transform
//       reg2->registered_op |= PDC_TRANSFORM;
//     }
//     PDC_Client_register_region_transform(userdefinedftn, loadpath, src_region_id, dest_region_id,
//       dest_object_id, current_state, thisFtn->nextState,
//       (int)PDC_DATA_MAP, (int)when, local_regIndex);
//   }
// 
// done:
//   if (applicationDir)
//     free(applicationDir);
//   if (userdefinedftn)
//     free(userdefinedftn);
// 
//   FUNC_LEAVE(ret_value);
// }

perr_t
PDCbuf_map_transform_register(char *func, void *buf, pdcid_t src_region_id, pdcid_t dest_object_id,
  pdcid_t dest_region_id, int current_state, int next_state,
  pdc_data_movement_t when)
{
  perr_t ret_value                                   = SUCCEED; /* Return value */
  void * ftnHandle                                   = NULL;
  size_t (*ftnPtr)()                                 = NULL;
  struct _pdc_obj_info *                 object1     = NULL;
  struct _pdc_region_transform_ftn_info *thisFtn     = NULL;
  struct pdc_region_info *               region_info = NULL;
  struct _pdc_id_info *                  id_info;
  char *                                 thisApp           = NULL;
  char *                                 colonsep          = NULL;
  char *                                 transformslibrary = NULL;
  char *                                 relpath = NULL;
  char *                                 userdefinedftn    = NULL;
  char *                                 dir_path          = NULL;
  char *                                 loadpath          = NULL;
  int                                    local_regIndex;

  FUNC_ENTER(NULL);

  thisApp = PDC_get_argv0_();
  if (thisApp)
  {
    dir_path = dirname(strdup(thisApp));
  }
  userdefinedftn = strdup(func);
  transformslibrary = default_pdc_transforms_lib;

  if ((colonsep = strrchr(userdefinedftn, ':')) != NULL) {
    *colonsep++       = 0;
    relpath = colonsep;
    transformslibrary = basename(relpath);
    dir_path = dirname(relpath);
  }
  loadpath = PDC_get_realpath(transformslibrary, dir_path);

  lib_t ldata;
  load_library_from_file(loadpath, &ldata);
  char tmp[PATH_MAX] = {};
  // snprintf(tmp, sizeof(tmp), "/tmp/%s", transformslibrary);
  snprintf(tmp, sizeof(tmp), "/dev/shm/%s", transformslibrary);
  // printf("write to %s\n", tmp);

  // if(!access(tmp, R_OK)) unlink(tmp);
  // int fd = open(tmp, O_CREAT|O_WRONLY|O_TRUNC, 0755);
  // size_t n = write(fd, ldata.data, ldata.size);
  // close(fd);

  // printf("transformslibrary: %s in %s\n", transformslibrary, loadpath);
  // if (PDC_get_ftnPtr_(userdefinedftn, tmp, &ftnHandle) < 0)
  if (PDC_get_ftnPtr_(userdefinedftn, loadpath, &ftnHandle) < 0)
    PGOTO_ERROR(FAIL, "PDC_get_ftnPtr_ returned an error!");

  if ((ftnPtr = ftnHandle) == NULL)
    PGOTO_ERROR(FAIL, "Transforms function lookup failed\n");

  printf("ftnPtr: %p\n", ftnPtr);

  if ((thisFtn = PDC_MALLOC(struct _pdc_region_transform_ftn_info)) == NULL)
    PGOTO_ERROR(FAIL, "PDC register_obj_transforms memory allocation failed");

  thisFtn->ftnPtr    = (size_t(*)())ftnPtr;
  thisFtn->object_id = dest_object_id;
  id_info            = PDC_find_id(src_region_id);
  if (id_info && ((region_info = (struct pdc_region_info *)id_info->obj_ptr) != NULL))
    thisFtn->src_region = region_info;

  id_info = PDC_find_id(dest_region_id);
  if (id_info && ((region_info = (struct pdc_region_info *)id_info->obj_ptr) != NULL))
    thisFtn->dest_region = region_info;

  // Flag the destination region with the transform
  // We do this here because the target region is what
  // will eventually be locked and then unlocked to enable
  // a mapping data transfer.
  region_info->registered_op |= PDC_TRANSFORM;

  thisFtn->op_type        = PDC_DATA_MAP;
  thisFtn->when           = when;
  thisFtn->lang           = C_lang;
  thisFtn->client_id      = pdc_client_mpi_rank_g;
  thisFtn->readyState     = current_state;
  thisFtn->ftn_lastResult = 0;
  thisFtn->data           = buf;
  id_info                 = PDC_find_id(dest_object_id);
  if (id_info)
    object1 = (struct _pdc_obj_info *)(id_info->obj_ptr);
  if (object1) {
    thisFtn->type = object1->obj_pt->obj_prop_pub->type;
    thisFtn->type_extent = PDC_get_var_type_size(object1->obj_pt->obj_prop_pub->type);
    thisFtn->dest_extent = PDC_get_var_type_size(object1->obj_pt->obj_prop_pub->type);
    thisFtn->dest_type   = object1->obj_pt->obj_prop_pub->type;
  }
  if (next_state == INCR_STATE)
    thisFtn->nextState = current_state + 1;
  else if (next_state == DECR_STATE)
    thisFtn->nextState = current_state - 1;
  else
    thisFtn->nextState = current_state;

  // Add to our own list of transforms functions
  if ((local_regIndex = PDC_add_transform_ptr_to_registry_(thisFtn)) < 0)
    PGOTO_ERROR(FAIL, "PDC unable to register transform function!");

  PDC_Client_register_region_transform(userdefinedftn, loadpath, src_region_id, dest_region_id,
    dest_object_id, current_state, thisFtn->nextState, (int)PDC_DATA_MAP,
    (int)when, local_regIndex);

done:
  if (userdefinedftn)
    free(userdefinedftn);
  if (loadpath)
    free(loadpath);

  FUNC_LEAVE(ret_value);
}

perr_t
PDCbuf_io_transform_register(char *func ATTRIBUTE(unused), void *buf ATTRIBUTE(unused),
  pdcid_t src_region_id ATTRIBUTE(unused), int current_state ATTRIBUTE(unused),
  int next_state ATTRIBUTE(unused), pdc_data_movement_t when ATTRIBUTE(unused))
{
  perr_t ret_value = FAIL; /* Return value (not implemented) */
#if 0
  void *ftnHandle = NULL;
  size_t (*ftnPtr)() = NULL;
  struct _pdc_obj_info *object1 = NULL;
  struct _pdc_region_transform_ftn_info *thisFtn = NULL;
  struct pdc_region_info *region_info;
  struct _pdc_id_info *id_info;
  char *thisApp = NULL;
  char *colonsep = NULL; 
  char *transformslibrary = NULL;
  char *applicationDir = NULL;
  char *userdefinedftn = NULL;
  char *loadpath = NULL;
  int local_regIndex;
#endif
  FUNC_ENTER(NULL);
  printf("IO transforms are not currently supported!\n");
  // done:
  //    if (applicationDir) free(applicationDir);
  //    if (userdefinedftn) free(userdefinedftn);

  FUNC_LEAVE(ret_value);
}

perr_t
PDC_transform_end()
{
  perr_t ret_value = SUCCEED;

  FUNC_ENTER(NULL);

  PDC_free_transform_registry();

  FUNC_LEAVE(ret_value);
}
