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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ENABLE_MPI
#include "mpi.h"
#endif

#include "pdc.h"

int main(int argc, char **argv) {
    pdcid_t pdc, create_prop, cont1, cont2, cont1_cp, cont2_cp;
    int rank = 0, size = 1;
    
#ifdef ENABLE_MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
#endif
    // create a pdc
    pdc = PDC_init("pdc");
    printf("create a new pdc\n");

    // create a container property
    create_prop = PDCprop_create(PDC_CONT_CREATE, pdc);
    if(create_prop > 0)
        printf("Create a container property\n");
    else
        printf("Fail to create container property @ line  %d!\n", __LINE__);

    // create a container
    cont1 = PDCcont_create("c1", create_prop);
    if(cont1 > 0)
        printf("Create a container c1\n");
    else
        printf("Fail to create container @ line  %d!\n", __LINE__);
       
    // create second container
    cont2 = PDCcont_create("c2", create_prop);
    if(cont2 > 0)
        printf("Create a container c2\n");
    else
        printf("Fail to create container @ line  %d!\n", __LINE__);

    // open 1st container
    cont1_cp = PDCcont_open("c1", pdc);
    if(cont1_cp == 0)
        printf("Fail to open container c1\n");
    else
        printf("Open container c1\n");

    // open 2nd container
    cont2_cp = PDCcont_open("c2", pdc);
    if(cont2_cp == 0)
        printf("Fail to open container c2\n");
    else
        printf("Open container c2 \n");

    // close cont1_cp
    if(PDCcont_close(cont1_cp) < 0)
        printf("fail to close container cont1_cp\n");
    else
        printf("successfully close container cont1_cp\n");

    // close cont2_cp
    if(PDCcont_close(cont2_cp) < 0)
        printf("fail to close container cont2_cp\n");
    else
        printf("successfully close container cont2_cp\n");

    // close cont1
    if(PDCcont_close(cont1) < 0)
        printf("fail to close container c1\n");
    else
        printf("successfully close container c1\n");

    // close cont2
    if(PDCcont_close(cont2) < 0)
        printf("fail to close container c2\n");
    else
        printf("successfully close container c2\n");

    // close a container property
    if(PDCprop_close(create_prop) < 0)
        printf("Fail to close property @ line %d\n", __LINE__);
    else
        printf("successfully close container property\n");

    // close pdc
    if(PDC_close(pdc) < 0)
       printf("fail to close PDC\n");
    else
       printf("PDC is closed\n");
    
#ifdef ENABLE_MPI
    MPI_Finalize();
#endif
    return 0;
}
