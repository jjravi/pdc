#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <inttypes.h>

/* #define ENABLE_MPI 1 */

#ifdef ENABLE_MPI
  #include "mpi.h"
#endif

#include <unistd.h>
#include <sys/time.h>
#include "pdc.h"
#include "pdc_client_connect.h"
#include "pdc_client_server_common.h"

void print_usage() {
    printf("Usage: srun -n ./data_server_read obj_name size_MB\n");
}

int main(int argc, char **argv)
{
    int rank = 0, size = 1;
    uint64_t size_MB, size_B;
    perr_t ret;
    int ndim = 1;

    struct timeval  pdc_timer_start;
    struct timeval  pdc_timer_end;
    double write_time = 0.0;
    pdcid_t global_obj = 0;
    pdcid_t local_obj = 0;
    pdcid_t local_region, global_region;

    uint64_t *offset; 
    uint64_t *mysize; 
    int i;
    float *mydata, *obj_data;
    char *obj_name;

    uint64_t my_data_size;
    uint64_t dims[1];

#ifdef ENABLE_MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
#endif

    if (argc < 3) {
        print_usage();
        return 0;
    }

    obj_name = argv[1];
    size_MB = atoi(argv[2]);

    if (rank == 0) {
        printf("Writing a %" PRIu64 " MB object [%s] with %d clients.\n", size_MB, obj_name, size);
    }
    size_B = size_MB * 1048576;

    // create a pdc
    pdcid_t pdc = PDC_init("pdc");
    /* printf("create a new pdc, pdc id is: %lld\n", pdc); */

    // create a container property
    pdcid_t cont_prop = PDCprop_create(PDC_CONT_CREATE, pdc);
    if(cont_prop <= 0)
        printf("Fail to create container property @ line  %d!\n", __LINE__);

    // create a container
    pdcid_t cont = PDCcont_create("c1", cont_prop);
    if(cont <= 0)
        printf("Fail to create container @ line  %d!\n", __LINE__);

    // create an object property
    pdcid_t obj_prop = PDCprop_create(PDC_OBJ_CREATE, pdc);
    if(obj_prop <= 0)
        printf("Fail to create object property @ line  %d!\n", __LINE__);

    dims[0] = size_B;
    my_data_size = size_B / size;

    obj_data = (float *)calloc(1, my_data_size);
    mydata = (float *)calloc(1, my_data_size);

    PDCprop_set_obj_type(obj_prop, PDC_FLOAT);
    PDCprop_set_obj_buf(obj_prop, obj_data);
    PDCprop_set_obj_dims(obj_prop, 1, dims);
    PDCprop_set_obj_user_id( obj_prop, getuid());
    PDCprop_set_obj_time_step( obj_prop, 0);
    PDCprop_set_obj_app_name(obj_prop, "DataServerTest");
    PDCprop_set_obj_tags(    obj_prop, "tag0=1");

    // Create a object 
    /* printf("Creating an object with name [%s]", obj_name); */
    /* fflush(stdout); */
    /* global_obj = PDCobj_create(cont, obj_name, obj_prop); */
    global_obj = PDCobj_create_mpi(cont, obj_name, obj_prop, 0);
    if (global_obj <= 0) {
        printf("Error creating an object [%s], exit...\n", obj_name);
        exit(-1);
    }

    /* printf("%d: object created.\n", rank); */
/* #ifdef ENABLE_MPI */
/*     MPI_Barrier(MPI_COMM_WORLD); */
/* #endif */

/*     // Query the created object */
/*     /1* printf("%d: Start to query object just created.\n", rank); *1/ */
/*     pdc_metadata_t *metadata; */
/*     PDC_Client_query_metadata_name_timestep_agg(obj_name, 0, &metadata); */
/*     /1* PDC_Client_query_metadata_name_timestep( obj_name, 0, &metadata); *1/ */
/*     /1* if (rank == 1) { *1/ */
/*     /1*     PDC_print_metadata(metadata); *1/ */
/*     /1* } *1/ */
/*     if (metadata == NULL || metadata->obj_id == 0) { */
/*         printf("Error with metadata!\n"); */
/*         exit(-1); */
/*     } */

    offset = (uint64_t*)malloc(sizeof(uint64_t) * ndim);
    mysize = (uint64_t*)malloc(sizeof(uint64_t) * ndim);
    offset[0] = rank * my_data_size;
    mysize[0] = my_data_size;

    local_region  = PDCregion_create(ndim, offset, mysize);
    global_region = PDCregion_create(ndim, offset, mysize);

    ret = PDCbuf_obj_map(mydata, PDC_FLOAT, local_region, global_obj, global_region);
    if(ret != SUCCEED) {
        printf("PDCbuf_obj_map failed\n");
        exit(-1);
    }
    /* printf("%d: writing to (%llu, %llu) of %llu bytes\n", rank, region.offset[0], region.offset[1], region.size[0]*region.size[1]); */

#ifdef ENABLE_MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif
    gettimeofday(&pdc_timer_start, 0);

    ret = PDCreg_obtain_lock(local_obj, local_region, WRITE, NOBLOCK);
    if (ret != SUCCEED) {
        printf("Failed to obtain lock for region\n");
        goto done;
    }

    ret = PDCreg_obtain_lock(global_obj, global_region, WRITE, NOBLOCK);
    if (ret != SUCCEED) {
        printf("Failed to obtain lock for region\n");
        goto done;
    }

    for (i = 0; i < 5; i++) {
        mydata[i] = i * 1.01;
    }

    ret = PDCreg_release_lock(local_obj, local_region, WRITE);
    if (ret != SUCCEED) {
        printf("Failed to release lock for region\n");
        goto done;
    }

    ret = PDCreg_release_lock(global_obj, global_region, WRITE);
    if (ret != SUCCEED) {
        printf("Failed to release lock for region\n");
        goto done;
    }

    /* PDC_Client_write(metadata, &region, mydata); */
    /* PDC_Client_write_wait_notify(metadata, &region, mydata); */

#ifdef ENABLE_MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif
    gettimeofday(&pdc_timer_end, 0);
    write_time = PDC_get_elapsed_time_double(&pdc_timer_start, &pdc_timer_end);

    if (rank == 0) { 
        printf("Time to lock and release data with %d ranks: %.6f\n", size, write_time);
        fflush(stdout);
    }

done:
    if(PDCobj_close(local_obj) < 0)
        printf("fail to close local obj\n");

    if(PDCobj_close(global_obj) < 0)
        printf("fail to close global obj\n");

    if(PDCregion_close(local_region) < 0)
        printf("fail to close local region\n");

    if(PDCregion_close(global_region) < 0)
        printf("fail to close global region\n");

    if(PDCcont_close(cont) < 0)
        printf("fail to close container\n");

    if(PDCprop_close(cont_prop) < 0)
        printf("Fail to close property @ line %d\n", __LINE__);

    if(PDC_close(pdc) < 0)
       printf("fail to close PDC\n");

#ifdef ENABLE_MPI
     MPI_Finalize();
#endif

     return 0;
}
