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

#include <filesystem>
#include <unordered_map>
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

/// NOTE: caller needs to free the buffer
void readFileC(const char *filepath, uint8_t **bytes, size_t &num_bytes)
{
  FILE *pFile = fopen(filepath , "rb");
  assert(pFile != NULL);

  // obtain file size:
  fseek (pFile, 0L, SEEK_END);
  num_bytes = ftell(pFile);
  fseek(pFile, 0L, SEEK_SET);
  assert(num_bytes > 0);

  // printf("size: %ld bytes\n", num_bytes);

  // allocate memory to contain the whole file:
  *bytes = (uint8_t*) malloc (sizeof(char)*num_bytes);
  // *bytes = (uint8_t *)calloc(num_bytes, sizeof(char));
  assert(*bytes != NULL);

  // copy the file into the *bytes:
  size_t result = fread (*bytes,1,num_bytes,pFile);
  assert(result == num_bytes);

  fclose (pFile);
}

typedef struct
{
  uint8_t *buf;
  size_t size;
} Memory;

#define NPARTICLES 8388608

int main(int argc, char **argv)
{
  int     mpi_rank = 0, mpi_size = 1;
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
  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
  MPI_Comm_dup(MPI_COMM_WORLD, &comm);
#endif

  printf("rank %d of %d\n", mpi_rank, mpi_size);

  if (argc < 2) {
    if (mpi_rank == 0)
      printf("Usage: %s data.f32 ...\n", argv[0]);
    return EXIT_FAILURE;
  }

  constexpr size_t dims[3] = {512, 512, 512};
  std::unordered_map<std::string, Memory> fields;

  for(int i = 1; i < argc; i++)
  {
    printf("reading files: %s\n", argv[i]);
    Memory m;
    readFileC(argv[i], (uint8_t**)&m.buf, m.size);
    fields.emplace(std::filesystem::path(argv[i]).stem().c_str(), m);
  }

  printf("available fields:\n");
  Memory m;
  for (const auto & [ key, value ] : fields)
  {
    const char *field_name = key.c_str();
    // Memory m = (Memory)value;
    m = (Memory)value;
    size_t bytes = m.size;
    printf("\t%s (%ld)\n", key.c_str(), bytes);
    assert(bytes == dims[0]*dims[1]*dims[2]*sizeof(float));
  }
  printf("\n");

  pdcid_t pdc_id = PDCinit("pdc");
  pdcid_t container_prop = PDCprop_create(PDC_CONT_CREATE, pdc_id);
  pdcid_t container_id = PDCcont_create_col("c1", container_prop);

  pdcid_t obj_prop_xx = PDCprop_create(PDC_OBJ_CREATE, pdc_id);
  PDCprop_set_obj_transfer_region_type(obj_prop_xx, PDC_REGION_STATIC);
  PDCprop_set_obj_dims(obj_prop_xx, 3, dims);
  PDCprop_set_obj_type(obj_prop_xx, PDC_FLOAT);
  PDCprop_set_obj_app_name(obj_prop_xx, (char *)std::string("NYX").c_str());

  pdcid_t obj_xx = PDCobj_create_mpi(container_id, "temperature", obj_prop_xx, 0, comm);

  constexpr int ndim = 3;
  constexpr uint64_t offset[3] = {0, 0, 0};
  constexpr uint64_t offset_remote[3] = {0, 0, 0};
  constexpr uint64_t mysize[3] = {512, 512, 512};

  // create a region
  pdcid_t region_x   = PDCregion_create(ndim, offset, mysize);
  pdcid_t region_xx   = PDCregion_create(ndim, offset_remote, mysize);

  pdcid_t transfer_request_x = PDCregion_transfer_create(&m.buf[0], PDC_WRITE, obj_xx, region_x, region_xx);
  PDC_API_CALL(PDCregion_transfer_start(transfer_request_x));
  PDC_API_CALL(PDCregion_transfer_wait(transfer_request_x));
  PDC_API_CALL(PDCregion_transfer_close(transfer_request_x));
  PDC_API_CALL(PDCobj_close(obj_xx));
  PDC_API_CALL(PDCregion_close(region_x));
  PDC_API_CALL(PDCregion_close(region_xx));
  PDC_API_CALL(PDCprop_close(obj_prop_xx));

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
