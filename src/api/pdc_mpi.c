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

#include "mpi.h"
#include "pdc_obj.h"

pdcid_t PDCobj_create_mpi(pdcid_t cont_id, const char *obj_name, pdcid_t obj_prop_id, int rank_id)
{
    pdcid_t ret_value = SUCCEED;
    struct PDC_obj_info *p = NULL;
    struct PDC_id_info *id_info = NULL;
    int rank;

    FUNC_ENTER(NULL);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if(rank == rank_id) {
        ret_value = pdc_obj_create(cont_id, obj_name, obj_prop_id, PDC_OBJ_GLOBAL);
    }
    else
        ret_value = pdc_obj_create(cont_id, obj_name, obj_prop_id, PDC_OBJ_LOCAL);

    id_info = pdc_find_id(ret_value);
    p = (struct PDC_obj_info *)(id_info->obj_ptr);
    p->client_id = rank;

    MPI_Bcast(&(p->meta_id), 1, MPI_LONG_LONG, rank_id, MPI_COMM_WORLD);

    FUNC_LEAVE(ret_value);
}

perr_t PDCobj_encode(pdcid_t obj_id, pdcid_t *meta_id)
{
    perr_t ret_value = FAIL;
    struct PDC_id_info *objinfo;
    struct PDC_obj_info *obj;
    int client_rank, client_size;

    FUNC_ENTER(NULL);

    MPI_Comm_size(MPI_COMM_WORLD, &client_size);
    if (client_size < 2) {
        PGOTO_ERROR(ret_value, "Requires at least two processes.");
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &client_rank);

    if(client_rank == 0) {
        objinfo = pdc_find_id(obj_id);
        if(objinfo == NULL)
            PGOTO_ERROR(ret_value, "cannot locate object ID");
        obj = (struct PDC_obj_info *)(objinfo->obj_ptr);
        if(obj->location == PDC_OBJ_LOCAL)
            PGOTO_ERROR(FAIL, "trying to encode local object");
        *meta_id = obj->meta_id;
    }

done:
    FUNC_LEAVE(ret_value);
}

pdcid_t PDCobj_decode(pdcid_t obj_id, pdcid_t meta_id)
{
    pdcid_t ret_value = 0;
    struct PDC_id_info *objinfo;
    struct PDC_obj_info *obj;
    int client_rank, client_size;

    FUNC_ENTER(NULL);

    MPI_Comm_size(MPI_COMM_WORLD, &client_size);
    if (client_size < 2) {
        PGOTO_ERROR(ret_value, "Requires at least two processes.");
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &client_rank);
    if(client_rank != 0) {
        objinfo = pdc_find_id(obj_id);
        if(objinfo == NULL)
            PGOTO_ERROR(ret_value, "cannot locate object ID");
        obj = (struct PDC_obj_info *)(objinfo->obj_ptr);
        obj->meta_id = meta_id;
    }

done:
    FUNC_LEAVE(ret_value);
}