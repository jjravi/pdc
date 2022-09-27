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

  if (argc != 2) {
    if (rank == 0) fprintf(stderr, "%s <filename>\n", argv[0]);
  }
  std::string filename(argv[1]);
  if (rank == 0) printf("Reading %s\n", filename.c_str());


  float *full = (float *)malloc(100 * 500 * 500 * sizeof(float));

  FILE *fp = fopen(filename.c_str(), "rb");
  fread(full, sizeof(float), 100*500*500, fp);
  fclose(fp);

  for(int i = 0; i < 10; i++)
  {
    // printf("%f ", full[i]);
  }
  // printf("\n");

  float *ts0 = (float *)malloc(500 * 500 * sizeof(float));

  for(int i = 0; i < 100; i++)
  {
    for(int j = 0; j < 500; j++)
    {
      for(int k = 0; k < 500; k++)
      {
        ts0[j*500 + k] = full[i*(500*500) + j*500 + k];
        // printf("%2.2f ", full[i*(500*500) + j*500 + k]);
        // if(k == 25) break;
      }
      // printf("\n");
    }
    break;
  }

  int num_subregions = size;

  int x_offset = 0;
  int y_offset = 0;

  switch(rank)
  {
    case 0:
      x_offset = 0;
      y_offset = 0;
      break;
    case 1:
      x_offset = 250;
      y_offset = 0;
      break;
    case 2:
      x_offset = 0;
      y_offset = 250;
      break;
    case 3:
      x_offset = 250;
      y_offset = 250;
      break;
    default:
      assert(false);
      break;
  }

  // printf("rank%d, x_offset: %d, y_offset: %d\n", rank, x_offset, y_offset);

  float *sub_region = (float *)malloc(250*250*sizeof(float));

  int xx = 0;
  for(int x = x_offset; x < x_offset+250; x++)
  {
    int yy = 0;
    for(int y = y_offset; y < y_offset+250; y++)
    {
      sub_region[x*250 + y] = ts0[x*500 + y];
      // if(rank == atoi(getenv("PRANK")))
        // printf("%2.2f ", ts0[x*500 + y]);
      yy++;
    }
    // if(rank == atoi(getenv("PRANK")))
      // printf("\n");
    xx++;
  }

  pdcid_t pdc_id = PDCinit("pdc");
 
  pdcid_t container_prop = PDCprop_create(PDC_CONT_CREATE, pdc_id);
  pdcid_t container_id = PDCcont_create_col("c1", container_prop);
  pdcid_t obj_prop_xx = PDCprop_create(PDC_OBJ_CREATE, pdc_id);

//   ///////////////////////////////////
//   // // issue 4 RPCs (these will proceed concurrently using callbacks)
//   // int req_num = 4;
//   // for (int i = 0; i < req_num; i++) run_my_rpc(i);
//   // printf("done issuing run_my_rpc\n");
//   ///////////////////////////////////
 
  uint64_t numparticles = NPARTICLES;
  float *x = (float *)malloc(numparticles * sizeof(float));

  uint64_t  dims[1] = {numparticles * size};
  PDCprop_set_obj_transfer_region_type(obj_prop_xx, PDC_REGION_STATIC);
  PDCprop_set_obj_dims(obj_prop_xx, 1, dims);
  PDCprop_set_obj_type(obj_prop_xx, PDC_FLOAT);
  PDCprop_set_obj_time_step(obj_prop_xx, 0);
  PDCprop_set_obj_user_id(obj_prop_xx, getuid());
  PDCprop_set_obj_app_name(obj_prop_xx, (char *)std::string("VPICIO").c_str());
  PDCprop_set_obj_tags(obj_prop_xx, (char *)std::string("tag0=1").c_str());
 
  for(int timestep = 0; timestep < 1; timestep++)
  {
    PDCprop_set_obj_time_step(obj_prop_xx, timestep);

    pdcid_t obj_xx = PDCobj_create_mpi(container_id,   ("obj-var-xx"+std::to_string(timestep)).c_str(), obj_prop_xx, 0, comm);

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
     pdcid_t region_xx   = PDCregion_create(ndim, offset_remote, mysize);
 
     PDC_API_CALL( PDCbuf_map_transform_register("pdc_sz_compress:libpdc_transform_sz.so", &x[0], region_x, obj_xx, region_xx, 0, INCR_STATE, DATA_OUT) );
 
     PDC_API_CALL( PDCbuf_obj_map(&x[0], PDC_FLOAT, region_x, obj_xx, region_xx) );
     PDC_API_CALL( PDCreg_obtain_lock(obj_xx, region_xx, PDC_WRITE, PDC_NOBLOCK) );
 
     // compute
     for (uint64_t i = 0; i < numparticles; i++) {
       x[i]   = uniform_random_number() * x_dim;
     }
 
     PDC_API_CALL( PDCreg_release_lock(obj_xx, region_xx, PDC_WRITE) );
     PDC_API_CALL( PDCbuf_obj_unmap(obj_xx, region_xx) );
 
     PDC_API_CALL(PDCregion_close(region_x));
     PDC_API_CALL(PDCregion_close(region_xx));
     PDC_API_CALL(PDCobj_close(obj_xx));
 
     free(offset);
     free(offset_remote);
     free(mysize);
 
 #ifdef ENABLE_MPI
     MPI_Barrier(MPI_COMM_WORLD);
 #endif
   }
 
  PDC_API_CALL(PDCprop_close(obj_prop_xx));
 
//   ///////////////////////////////////
//   // printf("call wait_my_rpc()\n");
//   // wait_my_rpc();
//   // printf("finish wait_my_rpc()\n");
//   ///////////////////////////////////

  PDC_API_CALL(PDCcont_close(container_id));
  PDC_API_CALL(PDCprop_close(container_prop));
  PDC_API_CALL(PDCclose(pdc_id));

#ifdef ENABLE_MPI
  MPI_Finalize();
#endif

  return 0;
}
