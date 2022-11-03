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

// #define NPARTICLES 8388608
#define NPARTICLES 100

double
uniform_random_number()
{
  return (((double)rand()) / ((double)(RAND_MAX)));
}

void
print_usage()
{
    printf("Usage: srun -n ./vpicio #particles\n");
}

int
main(int argc, char **argv)
{
  int rank = 0;
  int size = 1;

#ifdef ENABLE_MPI
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
#endif

  uint64_t numparticles = NPARTICLES;

  if (argc > 1)
    numparticles = atoll(argv[1]) * 1024 * 1024;

  if (rank == 0) {
    printf("Writing %" PRIu64 " number of particles with %d clients.\n", numparticles, size);
  }

  uint64_t dims[1];
  dims[0] = numparticles;
  float *fx = (float *)malloc(numparticles * sizeof(float));
  int *ix = (int *)malloc(numparticles * sizeof(int));

  pdcid_t pdc_id = PDCinit("pdc");

  pdcid_t container_prop = PDCprop_create(PDC_CONT_CREATE, pdc_id);
  pdcid_t container_id = PDCcont_create_col("c1", container_prop);
  pdcid_t obj_prop_fx = PDCprop_create(PDC_OBJ_CREATE, pdc_id);

  PDCprop_set_obj_dims(obj_prop_fx, 1, dims);
  PDCprop_set_obj_type(obj_prop_fx, PDC_FLOAT);
  PDCprop_set_obj_time_step(obj_prop_fx, 0);
  PDCprop_set_obj_user_id(obj_prop_fx, getuid());
  PDCprop_set_obj_app_name(obj_prop_fx, "VPICIO");
  PDCprop_set_obj_tags(obj_prop_fx, "tag0=1");

  // Dup and then set the obj datatype to PDC_INT
  pdcid_t obj_prop_ix = PDCprop_obj_dup(obj_prop_fx);
  PDCprop_set_obj_type(obj_prop_ix, PDC_INT);

  // Set the buf property of each to the actual data buffers
  PDCprop_set_obj_buf(obj_prop_fx, fx);
  PDCprop_set_obj_buf(obj_prop_ix, ix);

  char obj_name[64];
  sprintf(obj_name, "obj-var-fx-%d", rank);
  pdcid_t obj_fx = PDCobj_create(container_id, obj_name, obj_prop_fx);
  if (obj_fx == 0) {
    printf("Error getting an object id of %s from server, exit...\n", "obj-var-fx");
    exit(-1);
  }
  sprintf(obj_name, "obj-var-ix-%d", rank);
  pdcid_t obj_ix = PDCobj_create(container_id, obj_name, obj_prop_ix);
  if (obj_ix == 0) {
    printf("Error getting an object id of %s from server, exit...\n", "obj-var-ix");
    exit(-1);
  }

  int ndim = 1;
  uint64_t *offset = (uint64_t *)malloc(sizeof(uint64_t) * ndim);
  offset[0] = 0;

  uint64_t *offset_remote = (uint64_t *)malloc(sizeof(uint64_t) * ndim);
  offset_remote[0] = rank * numparticles;

  uint64_t *mysize = (uint64_t *)malloc(sizeof(uint64_t) * ndim);
  mysize[0] = numparticles;

  // create regions
  pdcid_t region_fx = PDCregion_create(ndim, offset, mysize);
  pdcid_t region_ix = PDCregion_create(ndim, offset, mysize);

  for (uint64_t i = 0; i < numparticles; i++) {
    fx[i] = uniform_random_number();
    ix[i] = i;
  }

#ifdef ENABLE_MPI
  MPI_Barrier(MPI_COMM_WORLD);
#endif

  // PDCobj_map(obj_fx, region_fx, obj_ix, region_ix);
  PDC_API_CALL( PDCbuf_obj_map(&fx[0], PDC_FLOAT, region_fx, obj_ix, region_ix) );

  // register a transform for when the mapping of 'obj_fx' takes place
  // PDC_API_CALL( PDCobj_transform_register("pdc_convert_datatype:libpdctransforms.so", obj_fx, 0, INCR_STATE, PDC_DATA_MAP, DATA_OUT) );
  PDC_API_CALL( PDCbuf_map_transform_register("pdc_convert_datatype:libpdctransforms.so", &fx[0], region_fx, obj_fx, region_ix, 0, INCR_STATE, DATA_OUT) );

#ifdef ENABLE_MPI
  MPI_Barrier(MPI_COMM_WORLD);
#endif

  PDC_API_CALL( PDCreg_obtain_lock(obj_ix, region_ix, PDC_WRITE, PDC_NOBLOCK) );

#ifdef ENABLE_MPI
  MPI_Barrier(MPI_COMM_WORLD);
#endif

  if (rank == 0) {
    printf("fx = ");
    for (uint64_t i = 0; i < 10; i++)
      printf(" %10.2lf", fx[i]);
    puts("\n----------------------");
  }

  PDC_API_CALL( PDCreg_release_lock(obj_ix, region_ix, PDC_WRITE) );

#ifdef ENABLE_MPI
  MPI_Barrier(MPI_COMM_WORLD);
#endif

  PDC_API_CALL( PDCbuf_obj_unmap(obj_ix, region_ix) );
  PDC_API_CALL( PDCobj_close(obj_fx) );
  PDC_API_CALL( PDCobj_close(obj_ix) );
  PDC_API_CALL( PDCprop_close(obj_prop_fx) );
  PDC_API_CALL( PDCprop_close(obj_prop_ix) );
  PDC_API_CALL( PDCregion_close(region_fx) );
  PDC_API_CALL( PDCregion_close(region_ix) );
  PDC_API_CALL( PDCcont_close(container_id) );
  PDC_API_CALL( PDCprop_close(container_prop) );
  PDC_API_CALL( PDCclose(pdc_id) );

  free(fx);
  free(ix);

#ifdef ENABLE_MPI
  MPI_Finalize();
#endif

  return 0;
}
