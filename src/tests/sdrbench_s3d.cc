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

  double *full = (double *)malloc(11 * 500 * 500 * 500 * sizeof(double));

  FILE *fp = fopen(filename.c_str(), "rb");
  fread(full, sizeof(double), 11*500*500*500, fp);
  fclose(fp);

  pdcid_t pdc_id = PDCinit("pdc");
 
  pdcid_t container_prop = PDCprop_create(PDC_CONT_CREATE, pdc_id);
  pdcid_t container_id = PDCcont_create_col("c1", container_prop);
  pdcid_t obj_prop_subrr = PDCprop_create(PDC_OBJ_CREATE, pdc_id);
  pdcid_t obj_prop_fullrr = PDCprop_create(PDC_OBJ_CREATE, pdc_id);

  uint64_t numparticles = 250*250*250;
  uint64_t  dims[1] = {numparticles * size};
  PDCprop_set_obj_transfer_region_type(obj_prop_subrr, PDC_REGION_STATIC);
  PDCprop_set_obj_dims(obj_prop_subrr, 1, dims);
  PDCprop_set_obj_type(obj_prop_subrr, PDC_DOUBLE);
  PDCprop_set_obj_time_step(obj_prop_subrr, 0);
  PDCprop_set_obj_user_id(obj_prop_subrr, getuid());
  PDCprop_set_obj_app_name(obj_prop_subrr, (char *)std::string("ISABEL").c_str());
  PDCprop_set_obj_tags(obj_prop_subrr, (char *)std::string("tag0=1").c_str());

  for(int ts_id = 0; ts_id < 11; ts_id++)
  {
    if(rank == 0) printf("ts_id=%d\n", ts_id);
    double (&ts0)[500][500][500] = *reinterpret_cast<double(*)[500][500][500]>(&full[ts_id]);

    int x_offset = 0;
    int y_offset = 0;
    int z_offset = 0;

    switch(rank)
    {
      case 0:
        x_offset = 250*0;
        y_offset = 250*0;
        z_offset = 250*0;
        break;
      case 1:
        x_offset = 250*1;
        y_offset = 250*0;
        z_offset = 250*0;
        break;
      case 2:
        x_offset = 250*0;
        y_offset = 250*1;
        z_offset = 250*0;
        break;
      case 3:
        x_offset = 250*1;
        y_offset = 250*1;
        z_offset = 250*0;
        break;
      case 4:
        x_offset = 250*0;
        y_offset = 250*0;
        z_offset = 250*1;
        break;
      case 5:
        x_offset = 250*1;
        y_offset = 250*0;
        z_offset = 250*1;
        break;
      case 6:
        x_offset = 250*0;
        y_offset = 250*1;
        z_offset = 250*1;
        break;
      case 7:
        x_offset = 250*1;
        y_offset = 250*1;
        z_offset = 250*1;
        break;
      default:
        assert(false);
        break;
    }

    double sub_region[250][250][250];

    for(int x = 0; x < 250; x++)
      for(int y = 0; y < 250; y++)
        for(int z = 0; z < 250; z++)
          sub_region[x][y][z] = ts0[x+x_offset][y+y_offset][z+z_offset];

    PDCprop_set_obj_time_step(obj_prop_subrr, ts_id);
    pdcid_t obj_xx = PDCobj_create_mpi(container_id,   ("obj-var-xx"+std::to_string(ts_id)).c_str(), obj_prop_subrr, 0, comm);

    int ndim = 1;
    uint64_t *offset = (uint64_t *)malloc(sizeof(uint64_t) * ndim);
    offset[0] = 0;

    uint64_t *offset_remote = (uint64_t *)malloc(sizeof(uint64_t) * ndim);
    offset_remote[0] = rank * numparticles;

    uint64_t *mysize = (uint64_t *)malloc(sizeof(uint64_t) * ndim);
    mysize[0] = numparticles;

    // create a region
    pdcid_t region_x   = PDCregion_create(ndim, offset, mysize);
    pdcid_t region_xx   = PDCregion_create(ndim, offset_remote, mysize);

    // PDC_API_CALL( PDCbuf_map_transform_register("pdc_cusz_compress:libpdc_transform_cusz.so", &sub_region[0], region_x, obj_xx, region_xx, 0, INCR_STATE, DATA_OUT) );
    // PDC_API_CALL( PDCbuf_map_transform_register("pdc_sz_compress:libpdc_transform_sz.so", &sub_region[0], region_x, obj_xx, region_xx, 0, INCR_STATE, DATA_OUT) );
    PDC_API_CALL( PDCbuf_map_transform_register("pdc_entropy:libanalyze_entropy.so", &sub_region[0], region_x, obj_xx, region_xx, 0, INCR_STATE, DATA_OUT) );

    MPI_Barrier(MPI_COMM_WORLD);
    PDC_API_CALL( PDCbuf_obj_map(&sub_region[0], PDC_DOUBLE, region_x, obj_xx, region_xx) );
    PDC_API_CALL( PDCreg_obtain_lock(obj_xx, region_xx, PDC_WRITE, PDC_NOBLOCK) );

    // TODO: compute
    sleep(1);

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
    if(rank == 0) printf("\n* * *\n");
    sleep(1);
    break;
  }

  PDC_API_CALL(PDCprop_close(obj_prop_subrr));
  PDC_API_CALL(PDCcont_close(container_id));
  PDC_API_CALL(PDCprop_close(container_prop));
  PDC_API_CALL(PDCclose(pdc_id));

#ifdef ENABLE_MPI
  MPI_Finalize();
#endif

  return 0;
}
