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

#ifndef _pdc_transform_support_H
#define _pdc_transform_support_H

#include "pdc_transforms_pkg.h"

/**
 * Register a function to be invoked at a specified point during execution
 * to transform the supplied data.
 *
 * \param func [IN]             String containing the [libraryname:]function to be registered.
 *                              (default library name = "libpdctransforms")
 * \param obj_id [IN]           PDC object id containing the input data.
 * \param current_state [IN]    State/Sequence ID to identify when the transform can take place.
 * \param next_state [IN]       State/Sequence ID after the transform is complete (should be +1 or -1).
 * \param op_type [IN]          An enumerated ID specifying an operation type that invokes the transform.
 * \param when [IN]             An enumerated ID specifying when/where a transform is invoked.
 *                              (examples for data movement: DATA_OUT, DATA_IN)
 *
 * \return Non-negative on success/Negative on failure
 */
perr_t PDCobj_transform_register(char *func, pdcid_t obj_id, int current_state, int next_state, PDCobj_transform_t op_type, PDCdata_movement_t when );

/**
 * Register a function to be invoked as a result of having mapped two regions.
 * The specfied transform function is invoked as a result of data movement between src and dest.
 *
 * \param func [IN]             String containing the [libraryname:]function to be registered.
 *                              (default library name = "libpdctransforms")
 * \param src_region_id [IN]    PDC region id of the data mapping source.
 * \param dest_region_id [IN]   PDC region id of the data mapping destination (target).
 * \param current_state [IN]    State/Sequence ID to identify when the transform can take place.
 * \param next_state [IN]       State/Sequence ID after the transform is complete (should be +1 or -1).
 * \param when [IN]             An enumerated ID specifying when/where a transform is invoked.
 *                              (examples for data movement: DATA_OUT, DATA_IN)
 *
 * \return Non-negative on success/Negative on failure
 */
perr_t PDCbuf_map_transform_register(char *func, void *buf, pdcid_t src_region_id, pdcid_t dest_object_id, pdcid_t dest_region_id, int current_state, int next_state, PDCdata_movement_t when );

#endif
