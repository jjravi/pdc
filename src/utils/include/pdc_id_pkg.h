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

#ifndef PDC_ID_PKG_H
#define PDC_ID_PKG_H

#include "pdc_private.h"
#include "pdc_linkedlist.h"
#include "mercury_atomic.h"

struct _pdc_id_info {
    pdcid_t           id;      /* ID for this info                 */
    hg_atomic_int32_t count;   /* ref. count for this atom         */
    void *            obj_ptr; /* pointer associated with the atom */
    PDC_LIST_ENTRY(_pdc_id_info) entry;
};

#endif /* PDC_ID_PKG_H */
