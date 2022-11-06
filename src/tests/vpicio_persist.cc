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

#include "my_rpc.h"
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

  // ////////////////////////////////
  // // issue 4 RPCs (these will proceed concurrently using callbacks)
  // int req_num = 4;
  // for (int i = 0; i < req_num; i++) run_my_rpc(i);
  // printf("done issuing run_my_rpc\n");
  // ////////////////////////////////

  ////////////////////////////////////////////////////
  PDCprop_set_obj_transfer_region_type(obj_prop_xx, PDC_OBJ_STATIC);
  // PDCprop_set_obj_transfer_region_type(obj_prop_xx, PDC_REGION_STATIC);
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

    pdcid_t transfer_request_x, transfer_request_y, transfer_request_z, transfer_request_px, transfer_request_py, transfer_request_pz, transfer_request_id1, transfer_request_id2; 

    pdcid_t transfers[8];

    pdcTransferCreate(&transfers[0], &transfer_request_x, &x[0], PDC_WRITE, obj_xx, region_x, region_xx);
    pdcTransferCreate(&transfers[1], &transfer_request_y, &y[0], PDC_WRITE, obj_yy, region_y, region_yy);
    pdcTransferCreate(&transfers[2], &transfer_request_z, &z[0], PDC_WRITE, obj_zz, region_z, region_zz);
    pdcTransferCreate(&transfers[3], &transfer_request_px, &px[0], PDC_WRITE, obj_pxx, region_px, region_pxx);
    pdcTransferCreate(&transfers[4], &transfer_request_py, &py[0], PDC_WRITE, obj_pyy, region_py, region_pyy);
    pdcTransferCreate(&transfers[5], &transfer_request_pz, &pz[0], PDC_WRITE, obj_pzz, region_pz, region_pzz);
    pdcTransferCreate(&transfers[6], &transfer_request_id1, &id1[0], PDC_WRITE, obj_id11, region_id1, region_id11);
    pdcTransferCreate(&transfers[7], &transfer_request_id2, &id2[0], PDC_WRITE, obj_id22, region_id2, region_id22);

    // ///////////////////
    // pdcTransfer_t transfer_x;
    // pdcTransferCreate(&transfer_x);
    // pdcTransferStart(transfer_x);
    // pdcTransferWait(transfer_x);
    // ///////////////////

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

    PDC_API_CALL(pdcTransferStart(transfer_request_x, transfers[0]));
    PDC_API_CALL(pdcTransferStart(transfer_request_y, transfers[1]));
    PDC_API_CALL(pdcTransferStart(transfer_request_z, transfers[2]));
    PDC_API_CALL(pdcTransferStart(transfer_request_px, transfers[3]));
    PDC_API_CALL(pdcTransferStart(transfer_request_py, transfers[4]));
    PDC_API_CALL(pdcTransferStart(transfer_request_pz, transfers[5]));
    PDC_API_CALL(pdcTransferStart(transfer_request_id1, transfers[6]));
    PDC_API_CALL(pdcTransferStart(transfer_request_id2, transfers[7]));

    PDC_API_CALL(pdcTransferWait(transfers[0]));
    PDC_API_CALL(pdcTransferWait(transfers[1]));
    PDC_API_CALL(pdcTransferWait(transfers[2]));
    PDC_API_CALL(pdcTransferWait(transfers[3]));
    PDC_API_CALL(pdcTransferWait(transfers[4]));
    PDC_API_CALL(pdcTransferWait(transfers[5]));
    PDC_API_CALL(pdcTransferWait(transfers[6]));
    PDC_API_CALL(pdcTransferWait(transfers[7]));

    PDC_API_CALL(PDCregion_transfer_wait(transfer_request_x));
    PDC_API_CALL(PDCregion_transfer_wait(transfer_request_y));
    PDC_API_CALL(PDCregion_transfer_wait(transfer_request_z));
    PDC_API_CALL(PDCregion_transfer_wait(transfer_request_px));
    PDC_API_CALL(PDCregion_transfer_wait(transfer_request_py));
    PDC_API_CALL(PDCregion_transfer_wait(transfer_request_pz));
    PDC_API_CALL(PDCregion_transfer_wait(transfer_request_id1));
    PDC_API_CALL(PDCregion_transfer_wait(transfer_request_id2));

    PDC_API_CALL(PDCregion_transfer_close(transfer_request_x));
    PDC_API_CALL(PDCregion_transfer_close(transfer_request_y));
    PDC_API_CALL(PDCregion_transfer_close(transfer_request_z));
    PDC_API_CALL(PDCregion_transfer_close(transfer_request_px));
    PDC_API_CALL(PDCregion_transfer_close(transfer_request_py));
    PDC_API_CALL(PDCregion_transfer_close(transfer_request_pz));
    PDC_API_CALL(PDCregion_transfer_close(transfer_request_id1));
    PDC_API_CALL(PDCregion_transfer_close(transfer_request_id2));

    /////////////////////////
    // pdcid_t input1_iter = PDCobj_data_iter_create(obj_id11, region_id1);

    // size_t data_dims[2] = {0,};
    // // pdcid_t obj11_iter = PDCobj_data_block_iterator_create(obj_id11, PDC_REGION_ALL, 3);

    // pdcid_t obj11_iter = PDCobj_data_block_iterator_create(obj_id11, region_id1, 3);
    // size_t elements_per_block;
    // int *checkData = NULL;

    // while((elements_per_block = PDCobj_data_getNextBlock(obj11_iter, (void **)&checkData, data_dims)) > 0) {
    //   size_t rows,column;
    //   int *next = checkData;
    //   for(rows=0; rows < data_dims[0]; rows++) {
    //     printf("\t%d\t%d\t%d\n", next[0], next[1], next[2]);
    //     next += 3;
    //   }
    //   puts("----------");
    // }
    /////////////////////////

    // size_t data_dims[2] = {0,};
    // // pdcid_t obj2_iter = PDCobj_data_block_iterator_create(obj2, PDC_REGION_ALL, 3);
    // pdcid_t obj2_iter = PDCobj_data_block_iterator_create(obj2, r2, 3);
    // size_t elements_per_block;
    // int *checkData = NULL;
    // while((elements_per_block = PDCobj_data_getNextBlock(obj2_iter, (void **)&checkData, data_dims)) > 0) {
    //   size_t rows,column;
    //   int *next = checkData;
    //   for(rows=0; rows < data_dims[0]; rows++) {
    //     printf("\t%d\t%d\t%d\n", next[0], next[1], next[2]);
    //     next += 3;
    //   }
    //   puts("----------");
    // }

    /////////////////////////

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

  // ///////////////////////////////////
  // printf("call wait_my_rpc()\n");
  // wait_my_rpc();
  // printf("finish wait_my_rpc()\n");
  // ///////////////////////////////////

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
