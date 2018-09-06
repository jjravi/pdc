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
#include <getopt.h>
#include <time.h>
#include "pdc.h"

int main() {
    pdcid_t pdc, cont_prop, cont, obj_prop1, obj_prop2, obj1, obj2;
    // create a pdc

    pdc = PDC_init("pdc");
    printf("create a new pdc\n");

    // create a container property
    cont_prop = PDCprop_create(PDC_CONT_CREATE, pdc);
    if(cont_prop > 0)
        printf("Create a container property\n");
    else
        printf("Fail to create container property @ line  %d!\n", __LINE__);

    // create a container
    cont = PDCcont_create("c1", cont_prop);
    if(cont > 0)
        printf("Create a container c1\n");
    else
        printf("Fail to create container @ line  %d!\n", __LINE__);
    
    // create an object property
    obj_prop1 = PDCprop_create(PDC_OBJ_CREATE, pdc);
    if(obj_prop1 > 0)
        printf("Create an object property\n");
    else
        printf("Fail to create object property @ line  %d!\n", __LINE__);

    obj_prop2 = PDCprop_create(PDC_OBJ_CREATE, pdc);
    if(obj_prop2 > 0)
        printf("Create an object property\n");
    else
        printf("Fail to create object property @ line  %d!\n", __LINE__);
    
    // create first object
    obj1 = PDCobj_create(cont, "o1", obj_prop1);
    if(obj1 > 0)
        printf("Create an object o1\n");
    else
        printf("Fail to create object @ line  %d!\n", __LINE__);
    
    // create second object
    obj2 = PDCobj_create(cont, "o2", obj_prop2);
    if(obj2 > 0)
        printf("Create an object o2\n");
    else
        printf("Fail to create object @ line  %d!\n", __LINE__);


    pdc_kvtag_t kvtag1, kvtag2, kvtag3;
    char *v1 = "value1";
    kvtag1.name = "key1string";
    kvtag1.value = (void*)v1;
    kvtag1.size  = strlen(v1)+1;

    int v2 = 2;
    kvtag2.name = "key2int";
    kvtag2.value = (void*)&v2;
    kvtag2.size  = sizeof(int);

    double v3 = 3.45;
    kvtag3.name = "key3double";
    kvtag3.value = (void*)&v3;
    kvtag3.size  = sizeof(double);

    if (PDC_add_kvtag(obj1, &kvtag1) < 0)
        printf("fail to add a kvtag to o1\n");
    else
        printf("successfully added a kvtag to o1\n");

    if (PDC_add_kvtag(obj2, &kvtag2) < 0)
        printf("fail to add a kvtag to o1\n");
    else
        printf("successfully added a kvtag to o1\n");

    if (PDC_add_kvtag(obj2, &kvtag3) < 0)
        printf("fail to add a kvtag to o1\n");
    else
        printf("successfully added a kvtag to o1\n");


    pdc_kvtag_t *value1, *value2, *value3;

    if (PDC_get_kvtag(obj1, kvtag1.name, &value1) < 0)
        printf("fail to get a kvtag from o1\n");
    else
        printf("successfully retrieved a kvtag [%s] = [%s] from o1\n", value1->name, (char*)value1->value);

    if (PDC_get_kvtag(obj2, kvtag2.name, &value2) < 0)
        printf("fail to get a kvtag from o2\n");
    else
        printf("successfully retrieved a kvtag [%s] = [%d] from o2\n", value2->name, *(int*)value2->value);

    if (PDC_get_kvtag(obj2, kvtag3.name, &value3) < 0)
        printf("fail to get a kvtag from o2\n");
    else
        printf("successfully retrieved a kvtag [%s] = [%f] from o2\n", value3->name, *(double*)value3->value);

    PDC_free_kvtag(&value1);

    if (PDC_get_kvtag(obj1, kvtag1.name, &value1) < 0)
        printf("fail to get a kvtag from o1\n");
    else
        printf("successfully retrieved a kvtag [%s] = [%s] from o1\n", value1->name, (char*)value1->value);


    PDC_free_kvtag(&value1);
    PDC_free_kvtag(&value2);
    PDC_free_kvtag(&value3);

    // close first object
    if(PDCobj_close(obj1) < 0)
        printf("fail to close object o1\n");
    else
        printf("successfully close object o1\n");
    
    // close second object
    if(PDCobj_close(obj2) < 0)
        printf("fail to close object o2\n");
    else
        printf("successfully close object o2\n");
       
    // close a container
    if(PDCcont_close(cont) < 0)
        printf("fail to close container c1\n");
    else
        printf("successfully close container c1\n");
    
    // close a container property
    if(PDCprop_close(obj_prop1) < 0)
        printf("Fail to close property @ line %d\n", __LINE__);
    else
        printf("successfully close object property\n");

    if(PDCprop_close(obj_prop2) < 0)
        printf("Fail to close property @ line %d\n", __LINE__);
    else
        printf("successfully close object property\n");

    if(PDCprop_close(cont_prop) < 0)
        printf("Fail to close property @ line %d\n", __LINE__);
    else
        printf("successfully close container property\n");

    // close pdc
    if(PDC_close(pdc) < 0)
       printf("fail to close PDC\n");
    /* else */
    /*    printf("PDC is closed\n"); */

     return 0;
}
