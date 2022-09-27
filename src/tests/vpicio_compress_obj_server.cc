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
#include "pdc_analysis_pkg.h"
#include "pdc_transforms_common_old.h"
#include "pdc_server_transform.h"
#include "pdc_analysis_and_transforms_common.h"

#ifdef ENABLE_MPI
#include "pdc_mpi.h"
#endif
}

#include <string>


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

  pdcid_t pdc_id = PDCinit("pdc");

  pdcid_t container_prop = PDCprop_create(PDC_CONT_CREATE, pdc_id);
  pdcid_t container_id = PDCcont_create_col("c1", container_prop);
  pdcid_t obj_prop_xx = PDCprop_create(PDC_OBJ_CREATE, pdc_id);

  ///////////////////////////////////
  // // issue 4 RPCs (these will proceed concurrently using callbacks)
  // int req_num = 4;
  // for (int i = 0; i < req_num; i++) run_my_rpc(i);
  // printf("done issuing run_my_rpc\n");
  ///////////////////////////////////

  ////////////////////////////////////////////////////
  // PDCprop_set_obj_transfer_region_type(obj_prop_xx, PDC_OBJ_STATIC);
  PDCprop_set_obj_transfer_region_type(obj_prop_xx, PDC_REGION_STATIC);
  // PDCprop_set_obj_transfer_region_type(obj_prop_xx, PDC_REGION_DYNAMIC);
  // PDCprop_set_obj_transfer_region_type(obj_prop_xx, PDC_REGION_LOCAL);
  ////////////////////////////////////////////////////

  PDCprop_set_obj_dims(obj_prop_xx, 1, dims);
  PDCprop_set_obj_type(obj_prop_xx, PDC_FLOAT);
  PDCprop_set_obj_time_step(obj_prop_xx, 0);
  PDCprop_set_obj_user_id(obj_prop_xx, getuid());
  PDCprop_set_obj_app_name(obj_prop_xx, (char *)std::string("VPICIO").c_str());
  PDCprop_set_obj_tags(obj_prop_xx, (char *)std::string("tag0=1").c_str());

  pdcid_t obj_prop_yy = PDCprop_obj_dup(obj_prop_xx);
  PDCprop_set_obj_type(obj_prop_yy, PDC_FLOAT);

  pdcid_t obj_prop_zz = PDCprop_obj_dup(obj_prop_xx);
  PDCprop_set_obj_type(obj_prop_zz, PDC_FLOAT);

  pdcid_t obj_prop_pxx = PDCprop_obj_dup(obj_prop_xx);
  PDCprop_set_obj_type(obj_prop_pxx, PDC_FLOAT);

  pdcid_t obj_prop_pyy = PDCprop_obj_dup(obj_prop_xx);
  PDCprop_set_obj_type(obj_prop_pyy, PDC_FLOAT);

  pdcid_t obj_prop_pzz = PDCprop_obj_dup(obj_prop_xx);
  PDCprop_set_obj_type(obj_prop_pzz, PDC_FLOAT);

  pdcid_t obj_prop_id11 = PDCprop_obj_dup(obj_prop_xx);
  PDCprop_set_obj_type(obj_prop_id11, PDC_INT);

  pdcid_t obj_prop_id22 = PDCprop_obj_dup(obj_prop_xx);
  PDCprop_set_obj_type(obj_prop_id22, PDC_INT);

  for(int timestep = 0; timestep < 1; timestep++)
  {
    PDCprop_set_obj_time_step(obj_prop_xx, timestep);
    PDCprop_set_obj_time_step(obj_prop_yy, timestep);
    PDCprop_set_obj_time_step(obj_prop_zz, timestep);
    PDCprop_set_obj_time_step(obj_prop_pxx, timestep);
    PDCprop_set_obj_time_step(obj_prop_pyy, timestep);
    PDCprop_set_obj_time_step(obj_prop_pzz, timestep);
    PDCprop_set_obj_time_step(obj_prop_id11, timestep);
    PDCprop_set_obj_time_step(obj_prop_id22, timestep);


    pdcid_t obj_xx = PDCobj_create_mpi(container_id,   ("obj-var-xx"+std::to_string(timestep)).c_str(), obj_prop_xx, 0, comm);
    pdcid_t obj_yy = PDCobj_create_mpi(container_id,   ("obj-var-yy"+std::to_string(timestep)).c_str(), obj_prop_yy, 0, comm);
    pdcid_t obj_zz = PDCobj_create_mpi(container_id,   ("obj-var-zz"+std::to_string(timestep)).c_str(), obj_prop_zz, 0, comm);
    pdcid_t obj_pxx = PDCobj_create_mpi(container_id,  ("obj-var-pxx"+std::to_string(timestep)).c_str(), obj_prop_pxx, 0, comm);
    pdcid_t obj_pyy = PDCobj_create_mpi(container_id,  ("obj-var-pyy"+std::to_string(timestep)).c_str(), obj_prop_pyy, 0, comm);
    pdcid_t obj_pzz = PDCobj_create_mpi(container_id,  ("obj-var-pzz"+std::to_string(timestep)).c_str(), obj_prop_pzz, 0, comm);
    pdcid_t obj_id11 = PDCobj_create_mpi(container_id, ("id11"+std::to_string(timestep)).c_str(), obj_prop_id11, 0, comm);
    pdcid_t obj_id22 = PDCobj_create_mpi(container_id, ("id22"+std::to_string(timestep)).c_str(), obj_prop_id22, 0, comm);

    int ndim = 1;
    uint64_t *offset = (uint64_t *)malloc(sizeof(uint64_t) * ndim);
    offset[0] = 0;

    uint64_t *offset_remote = (uint64_t *)malloc(sizeof(uint64_t) * ndim);
    offset_remote[0] = rank * numparticles;

    uint64_t *mysize = (uint64_t *)malloc(sizeof(uint64_t) * ndim);
    mysize[0] = numparticles;

    //////////////////////////////////////////////////////////////////////////////////////////
    // register
    // PDC_API_CALL( PDCobj_transform_register("pdc_transform_increment", obj_id11, 0, INCR_STATE, PDC_DATA_MAP, DATA_OUT) );
    //////////////////////////////////////////////////////////////////////////////////////////

    // create a region
    pdcid_t region_x   = PDCregion_create(ndim, offset, mysize);
    pdcid_t region_y   = PDCregion_create(ndim, offset, mysize);
    pdcid_t region_z   = PDCregion_create(ndim, offset, mysize);
    pdcid_t region_px  = PDCregion_create(ndim, offset, mysize);
    pdcid_t region_py  = PDCregion_create(ndim, offset, mysize);
    pdcid_t region_pz  = PDCregion_create(ndim, offset, mysize);
    pdcid_t region_id1 = PDCregion_create(ndim, offset, mysize);
    pdcid_t region_id2 = PDCregion_create(ndim, offset, mysize);

    pdcid_t region_xx   = PDCregion_create(ndim, offset_remote, mysize);
    pdcid_t region_yy   = PDCregion_create(ndim, offset_remote, mysize);
    pdcid_t region_zz   = PDCregion_create(ndim, offset_remote, mysize);
    pdcid_t region_pxx  = PDCregion_create(ndim, offset_remote, mysize);
    pdcid_t region_pyy  = PDCregion_create(ndim, offset_remote, mysize);
    pdcid_t region_pzz  = PDCregion_create(ndim, offset_remote, mysize);
    pdcid_t region_id11 = PDCregion_create(ndim, offset_remote, mysize);
    pdcid_t region_id22 = PDCregion_create(ndim, offset_remote, mysize);

    pdcid_t iter1 = PDCobj_data_iter_create(obj_xx, region_x);
    pdcid_t iter2 = PDCobj_data_iter_create(obj_yy, region_y);

    struct _pdc_iterator_info * i_iter = &PDC_Block_iterator_cache[iter1];
    struct _pdc_iterator_info * o_iter = &PDC_Block_iterator_cache[iter2];

    // PDC_API_CALL( PDCbuf_map_transform_register("pdc_transform_increment", &id1[0], region_id1, obj_id11, region_id11, 0, INCR_STATE, DATA_OUT) );
    // PDC_API_CALL( PDCbuf_map_transform_register("pdc_transform_compress", &id1[0], region_id1, obj_id11, region_id11, 0, INCR_STATE, DATA_OUT) );
    // PDC_API_CALL( PDCbuf_map_transform_register("pdc_passthrough", &id1[0], region_id1, obj_id11, region_id11, 0, INCR_STATE, DATA_OUT) );
    // PDC_API_CALL( PDCbuf_map_transform_register("pdc_passthrough:libpdc_transform_test.so", &id1[0], region_id1, obj_id11, region_id11, 0, INCR_STATE, DATA_OUT) );
    // PDC_API_CALL( PDCbuf_map_transform_register("pdc_sz_compress:libpdc_transform_sz.so", &id1[0], region_id1, obj_id11, region_id11, 0, INCR_STATE, DATA_OUT) );
    // PDC_API_CALL( PDCbuf_map_transform_register("pdc_sz_compress:libpdc_transform_sz.so", &x[0], region_x, obj_xx, region_xx, 0, INCR_STATE, DATA_OUT) );
    PDC_API_CALL( PDCobj_analysis_register("demo_sum", iter1, iter2) );

    PDC_API_CALL( PDCbuf_obj_map(&x[0], PDC_FLOAT, region_x, obj_xx, region_xx) );
    PDC_API_CALL( PDCbuf_obj_map(&y[0], PDC_FLOAT, region_y, obj_yy, region_yy) );
    PDC_API_CALL( PDCbuf_obj_map(&z[0], PDC_FLOAT, region_z, obj_zz, region_zz) );
    PDC_API_CALL( PDCbuf_obj_map(&px[0], PDC_FLOAT, region_px, obj_pxx, region_pxx) );
    PDC_API_CALL( PDCbuf_obj_map(&py[0], PDC_FLOAT, region_py, obj_pyy, region_pyy) );
    PDC_API_CALL( PDCbuf_obj_map(&pz[0], PDC_FLOAT, region_pz, obj_pzz, region_pzz) );
    PDC_API_CALL( PDCbuf_obj_map(&id1[0], PDC_INT, region_id1, obj_id11, region_id11) );
    PDC_API_CALL( PDCbuf_obj_map(&id2[0], PDC_INT, region_id2, obj_id22, region_id22) );

    PDC_API_CALL( PDCreg_obtain_lock(obj_xx, region_xx, PDC_WRITE, PDC_NOBLOCK) );
    PDC_API_CALL( PDCreg_obtain_lock(obj_yy, region_yy, PDC_WRITE, PDC_NOBLOCK) );
    PDC_API_CALL( PDCreg_obtain_lock(obj_zz, region_zz, PDC_WRITE, PDC_NOBLOCK) );
    PDC_API_CALL( PDCreg_obtain_lock(obj_pxx, region_pxx, PDC_WRITE, PDC_NOBLOCK) );
    PDC_API_CALL( PDCreg_obtain_lock(obj_pyy, region_pyy, PDC_WRITE, PDC_NOBLOCK) );
    PDC_API_CALL( PDCreg_obtain_lock(obj_pzz, region_pzz, PDC_WRITE, PDC_NOBLOCK) );
    PDC_API_CALL( PDCreg_obtain_lock(obj_id11, region_id11, PDC_WRITE, PDC_NOBLOCK) );
    PDC_API_CALL( PDCreg_obtain_lock(obj_id22, region_id22, PDC_WRITE, PDC_NOBLOCK) );

    // compute
    for (uint64_t i = 0; i < numparticles; i++) {
      id1[i] = i;
      id2[i] = i * 2;
      x[i]   = uniform_random_number() * x_dim;
      y[i]   = uniform_random_number() * y_dim;
      z[i]   = ((float)id1[i] / numparticles) * z_dim;
      px[i]  = uniform_random_number() * x_dim;
      py[i]  = uniform_random_number() * y_dim;
      pz[i]  = ((float)id2[i] / numparticles) * z_dim;
    }

    PDC_API_CALL( PDCreg_release_lock(obj_xx, region_xx, PDC_WRITE) );
    PDC_API_CALL( PDCreg_release_lock(obj_yy, region_yy, PDC_WRITE) );
    PDC_API_CALL( PDCreg_release_lock(obj_zz, region_zz, PDC_WRITE) );
    PDC_API_CALL( PDCreg_release_lock(obj_pxx, region_pxx, PDC_WRITE) );
    PDC_API_CALL( PDCreg_release_lock(obj_pyy, region_pyy, PDC_WRITE) );
    PDC_API_CALL( PDCreg_release_lock(obj_pzz, region_pzz, PDC_WRITE) );
    PDC_API_CALL( PDCreg_release_lock(obj_id11, region_id11, PDC_WRITE) );
    PDC_API_CALL( PDCreg_release_lock(obj_id22, region_id22, PDC_WRITE) );

    PDC_API_CALL( PDCbuf_obj_unmap(obj_xx, region_xx) );
    PDC_API_CALL( PDCbuf_obj_unmap(obj_yy, region_yy) );
    PDC_API_CALL( PDCbuf_obj_unmap(obj_zz, region_zz) );
    PDC_API_CALL( PDCbuf_obj_unmap(obj_pxx, region_pxx) );
    PDC_API_CALL( PDCbuf_obj_unmap(obj_pyy, region_pyy) );
    PDC_API_CALL( PDCbuf_obj_unmap(obj_pzz, region_pzz) );
    PDC_API_CALL( PDCbuf_obj_unmap(obj_id11, region_id11) );
    PDC_API_CALL( PDCbuf_obj_unmap(obj_id22, region_id22) );

    PDC_API_CALL(PDCregion_close(region_x));
    PDC_API_CALL(PDCregion_close(region_y));
    PDC_API_CALL(PDCregion_close(region_z));
    PDC_API_CALL(PDCregion_close(region_px));
    PDC_API_CALL(PDCregion_close(region_py));
    PDC_API_CALL(PDCregion_close(region_pz));
    PDC_API_CALL(PDCregion_close(region_id1));
    PDC_API_CALL(PDCregion_close(region_id2));

    PDC_API_CALL(PDCregion_close(region_xx));
    PDC_API_CALL(PDCregion_close(region_yy));
    PDC_API_CALL(PDCregion_close(region_zz));
    PDC_API_CALL(PDCregion_close(region_pxx));
    PDC_API_CALL(PDCregion_close(region_pyy));
    PDC_API_CALL(PDCregion_close(region_pzz));
    PDC_API_CALL(PDCregion_close(region_id11));
    PDC_API_CALL(PDCregion_close(region_id22));

    PDC_API_CALL(PDCobj_close(obj_xx));
    PDC_API_CALL(PDCobj_close(obj_yy));
    PDC_API_CALL(PDCobj_close(obj_zz));
    PDC_API_CALL(PDCobj_close(obj_pxx));
    PDC_API_CALL(PDCobj_close(obj_pyy));
    PDC_API_CALL(PDCobj_close(obj_pzz));
    PDC_API_CALL(PDCobj_close(obj_id11));
    PDC_API_CALL(PDCobj_close(obj_id22));

    free(offset);
    free(offset_remote);
    free(mysize);

#ifdef ENABLE_MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif
  }

  PDC_API_CALL(PDCprop_close(obj_prop_xx));
  PDC_API_CALL(PDCprop_close(obj_prop_yy));
  PDC_API_CALL(PDCprop_close(obj_prop_zz));
  PDC_API_CALL(PDCprop_close(obj_prop_pxx));
  PDC_API_CALL(PDCprop_close(obj_prop_pyy));
  PDC_API_CALL(PDCprop_close(obj_prop_pzz));
  PDC_API_CALL(PDCprop_close(obj_prop_id11));
  PDC_API_CALL(PDCprop_close(obj_prop_id22));

  ///////////////////////////////////
  // printf("call wait_my_rpc()\n");
  // wait_my_rpc();
  // printf("finish wait_my_rpc()\n");
  ///////////////////////////////////

  PDC_API_CALL(PDCcont_close(container_id));
  PDC_API_CALL(PDCprop_close(container_prop));
  PDC_API_CALL(PDCclose(pdc_id));

  free(x);
  free(y);
  free(z);
  free(px);
  free(py);
  free(pz);
  free(id1);
  free(id2);

#ifdef ENABLE_MPI
  MPI_Finalize();
#endif

  return 0;
}
