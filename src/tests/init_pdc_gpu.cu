/**
 * @file
 * @author John J. Ravi (jjravi)
 *
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
#include <string>

#define PDC_API_CALL(apiFuncCall)                                        \
{                                                                        \
  perr_t _status = apiFuncCall;                                          \
  if (_status != SUCCEED) {                                              \
    fprintf(stderr, "%s:%d: error: function %s failed with error %d.\n", \
      __FILE__, __LINE__, #apiFuncCall, _status);                        \
    exit(-1);                                                            \
  }                                                                      \
}

#define CUDA_RUNTIME_API_CALL(apiFuncCall)                               \
{                                                                        \
  cudaError_t _status = apiFuncCall;                                     \
  if (_status != cudaSuccess) {                                          \
    fprintf(stderr, "%s:%d: error: function %s failed with error %s.\n", \
      __FILE__, __LINE__, #apiFuncCall, cudaGetErrorString(_status));    \
    exit(-1);                                                            \
  }                                                                      \
}

#define NPARTICLES 8388608

double
uniform_random_number()
{
  return (((double)rand()) / ((double)(RAND_MAX)));
}

int main(int argc, char **argv)
{
  int     rank = 0, size = 1;
  perr_t  ret;
#ifdef ENABLE_MPI
  MPI_Comm comm;
#else
  int comm = 1;
#endif
  int       x_dim = 64;
  int       y_dim = 64;
  int       z_dim = 64;

#ifdef ENABLE_MPI
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_dup(MPI_COMM_WORLD, &comm);
#endif

  srand(0);

  uint64_t numparticles = NPARTICLES;
  if (argc == 2) {
    numparticles = atoll(argv[1]);
    if (rank == 0)
      printf("Writing %" PRIu64 " number of particles with %d clients.\n", numparticles, size);
  }

  uint64_t  dims[1] = {numparticles * size};

  float *x = (float *)malloc(numparticles * sizeof(float));
  float *y = (float *)malloc(numparticles * sizeof(float));
  float *z = (float *)malloc(numparticles * sizeof(float));

  float *px = (float *)malloc(numparticles * sizeof(float));
  float *py = (float *)malloc(numparticles * sizeof(float));
  float *pz = (float *)malloc(numparticles * sizeof(float));

  int *id1 = (int *)malloc(numparticles * sizeof(int));
  int *id2 = (int *)malloc(numparticles * sizeof(int));

  float *d_x;
  float *d_y;
  float *d_z;

  float *d_px;
  float *d_py;
  float *d_pz;

  float *d_id1;
  float *d_id2;

  CUDA_RUNTIME_API_CALL( cudaSetDevice(0) );

  CUDA_RUNTIME_API_CALL( cudaMallocManaged((void **)&d_x, numparticles*sizeof(float)) );

  // compute
  for (uint64_t i = 0; i < numparticles; i++) {
    d_x[i]   = i;
  }

  pdcid_t pdc_id = PDCinit("pdc");

  pdcid_t container_prop = PDCprop_create(PDC_CONT_CREATE, pdc_id);
  pdcid_t container_id = PDCcont_create_col("c1", container_prop);
  pdcid_t obj_prop_xx = PDCprop_create(PDC_OBJ_CREATE, pdc_id);
  PDCprop_set_obj_transfer_region_type(obj_prop_xx, PDC_REGION_STATIC);

  PDCprop_set_obj_dims(obj_prop_xx, 1, dims);
  PDCprop_set_obj_type(obj_prop_xx, PDC_FLOAT);
  PDCprop_set_obj_time_step(obj_prop_xx, 0);
  PDCprop_set_obj_user_id(obj_prop_xx, getuid());
  PDCprop_set_obj_app_name(obj_prop_xx, (char *)std::string("VPICIO").c_str());
  PDCprop_set_obj_tags(obj_prop_xx, (char *)std::string("tag0=1").c_str());

  PDCprop_set_obj_time_step(obj_prop_xx, 0);

  pdcid_t obj_xx = PDCobj_create_mpi(container_id,   ("obj-var-xx"+std::to_string(0)).c_str(), obj_prop_xx, 0, comm);

  int ndim = 1;
  uint64_t *offset = (uint64_t *)malloc(sizeof(uint64_t) * ndim);
  offset[0] = 0;

  uint64_t *offset_remote = (uint64_t *)malloc(sizeof(uint64_t) * ndim);
  offset_remote[0] = rank * numparticles;

  uint64_t *mysize = (uint64_t *)malloc(sizeof(uint64_t) * ndim);
  mysize[0] = numparticles;

  pdcid_t region_x   = PDCregion_create(ndim, offset, mysize);
  pdcid_t region_xx   = PDCregion_create(ndim, offset_remote, mysize);
  // PDC_API_CALL( PDCbuf_map_transform_register("pdc_cusz_compress:libpdc_transform_cusz.so", &d_x[0], region_x, obj_xx, region_xx, 0, INCR_STATE, DATA_OUT) );
  PDC_API_CALL( PDCbuf_map_transform_register("pdc_entropy:libanalyze_entropy.so", &d_x[0], region_x, obj_xx, region_xx, 0, INCR_STATE, DATA_OUT) );

  PDC_API_CALL( PDCbuf_obj_map(&d_x[0], PDC_FLOAT, region_x, obj_xx, region_xx) );
  PDC_API_CALL( PDCreg_obtain_lock(obj_xx, region_xx, PDC_WRITE, PDC_NOBLOCK) );

  // for (uint64_t i = 0; i < numparticles; i++) {
  //   x[i]   = uniform_random_number() * x_dim;
  // }
  // CUDA_RUNTIME_API_CALL( cudaMemcpy(d_x,   x, numparticles*sizeof(float), cudaMemcpyHostToDevice) );
  
  PDC_API_CALL( PDCreg_release_lock(obj_xx, region_xx, PDC_WRITE) );

  PDC_API_CALL( PDCbuf_obj_unmap(obj_xx, region_xx) );

  PDC_API_CALL(PDCregion_close(region_x));

  PDC_API_CALL(PDCregion_close(region_xx));

  PDC_API_CALL(PDCobj_close(obj_xx));

  free(offset);
  free(offset_remote);
  free(mysize);

  PDC_API_CALL(PDCprop_close(obj_prop_xx));
  PDC_API_CALL(PDCcont_close(container_id));
  PDC_API_CALL(PDCprop_close(container_prop));
  PDC_API_CALL(PDCclose(pdc_id));

  free(x);

#ifdef ENABLE_MPI
  MPI_Finalize();
#endif

  return 0;
}
