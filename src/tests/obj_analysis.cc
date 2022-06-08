#include <mpi.h>

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <inttypes.h>

#include "pdc.h"
#include "pdc_transform.h"
#include "pdc_analysis_pkg.h"
#include "pdc_transforms_common_old.h"
#include "pdc_server_transform.h"
#include "pdc_analysis_and_transforms_common.h"

#ifdef ENABLE_MPI
#include "pdc_mpi.h"
#endif
}

#undef NDEBUG // enable asserts on release build
#include <assert.h>


#define PDC_API_CALL(apiFuncCall)                                        \
{                                                                        \
  perr_t _status = apiFuncCall;                                          \
  if (_status != SUCCEED) {                                              \
    fprintf(stderr, "%s:%d: error: function %s failed with error %d.\n", \
      __FILE__, __LINE__, #apiFuncCall, _status);                        \
    exit(-1);                                                            \
  }                                                                      \
}

int main(int argc, char **argv)
{
#ifdef ENABLE_MPI
  MPI_Comm comm;
#endif
  int rank = 0, size = 1;
  perr_t ret = SUCCEED;
  pdcid_t result1_prop, result2_prop;
  pdcid_t result1;

  int myresult1[2];     /*  sum{6,7}, sum{10,11} */

  uint64_t offset0[2] = {0, 0};
  uint64_t offset[2] = {1, 1};
  uint64_t rdims[2] = {2, 2};
  uint64_t array1_dims[2] = {4,4};
  int myArray1[4][4] = {
    {1,  2, 3, 4},
    {5,  6, 7, 8},
    {9, 10,11,12},
    {13,14,15,16}
  };

  uint64_t result1_dims[1] = {2};

#ifdef ENABLE_MPI
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_dup(MPI_COMM_WORLD, &comm);
#endif

  pdcid_t pdc_id = PDCinit("pdc");
  pdcid_t container_prop = PDCprop_create(PDC_CONT_CREATE, pdc_id);
  pdcid_t container_id = PDCcont_create_col("c1", container_prop);

  pdcid_t obj1_prop = PDCprop_create(PDC_OBJ_CREATE, pdc_id);
  PDCprop_set_obj_dims     (obj1_prop, 2, array1_dims);
  PDCprop_set_obj_type     (obj1_prop, PDC_INT );
  PDCprop_set_obj_time_step(obj1_prop, 0       );
  PDCprop_set_obj_user_id  (obj1_prop, getuid());
  PDCprop_set_obj_app_name (obj1_prop, "object_analysis" );
  PDCprop_set_obj_tags(     obj1_prop, "tag0=1");

  pdcid_t obj2_prop = PDCprop_obj_dup(obj1_prop);
  PDCprop_set_obj_type(obj2_prop, PDC_INT); /* Dup doesn't replicate the datatype */

  pdcid_t obj1 = PDCobj_create_mpi(container_id, "obj-var-array1", obj1_prop, 0, comm);
  if (obj1 == 0) {
    printf("Error getting an object id of %s from server, exit...\n", "obj-var-array1");
    exit(-1);
  }

  pdcid_t obj2 = PDCobj_create_mpi(container_id, "obj-var-result1", obj2_prop, 0, comm);
  if (obj2 == 0) {
    printf("Error getting an object id of %s from server, exit...\n", "obj-var-result1");
    exit(-1);
  }

  // create regions
  pdcid_t r1 = PDCregion_create(2, offset0, array1_dims);
  pdcid_t r2 = PDCregion_create(2, offset0, array1_dims);

  pdcid_t input1_iter = PDCobj_data_iter_create(obj1, r1);
  pdcid_t result1_iter = PDCobj_data_iter_create(obj2, r2);

  struct _pdc_iterator_info * i_iter = &PDC_Block_iterator_cache[input1_iter];
  struct _pdc_iterator_info * o_iter = &PDC_Block_iterator_cache[result1_iter];

  /* Need to register the analysis function PRIOR TO establishing
   * the region mapping.  The obj_mapping will update the region
   * structures to indicate that an analysis operation has been
   * added to the lock release...
   */
  PDCobj_analysis_register("pdc_server_passthrough:libpdc_server_transform_test.so", input1_iter, result1_iter);
  PDC_API_CALL( PDCbuf_obj_map(&myArray1[0], PDC_INT, r1, obj2, r2) );
  PDC_API_CALL( PDCreg_obtain_lock(obj1, r1, PDC_WRITE, PDC_NOBLOCK) );
  PDC_API_CALL( PDCreg_release_lock(obj1, r1, PDC_WRITE) );

#ifdef ENABLE_MPI
  MPI_Barrier(MPI_COMM_WORLD);
#endif

  PDC_API_CALL(PDCcont_close(container_id));
  PDC_API_CALL(PDCprop_close(container_prop));
  PDC_API_CALL(PDCclose(pdc_id));

#ifdef ENABLE_MPI
  MPI_Finalize();
#endif

  return 0;
}
