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

#ifndef PDC_REGION_H
#define PDC_REGION_H

#include "pdc_public.h"
#include "pdc_obj.h"

/**************************/
/* Library Public Struct */
/**************************/
struct pdc_region_info {
    pdcid_t               local_id;
    struct _pdc_obj_info *obj;
    size_t                ndim;
    uint64_t *            dims_size; /**< size of each dim, up to 3 */
    uint64_t *            offset;
    bool                  mapping;
    int                   registered_op;
    void *                data_buf; /**< data buffer */
    uint64_t              data_size; /**< Size of data buffer */
    size_t                unit; /**< Size of data type */
};

typedef enum {
    PDC_TRANSFER_STATUS_COMPLETE  = 0,
    PDC_TRANSFER_STATUS_PENDING   = 1,
    PDC_TRANSFER_STATUS_NOT_FOUND = 2
} pdc_transfer_status_t;

// pdc region transfer class. Contains essential information for performing non-blocking PDC client I/O
// perations.
typedef struct pdc_transfer_request {
    pdcid_t obj_id;
    // Data server ID for sending data to, used by object static only.
    uint32_t data_server_id;
    // Metadata server ID for sending data to, used by region_dynamic only.
    uint32_t metadata_server_id;
    // List of metadata. For dynamic object partitioning strategy, the metadata_id are owned by obj_servers
    // correspondingly. For static object partitioning, this ID is managed by the server with data_server_id.
    uint64_t *metadata_id;
    // PDC_READ or PDC_WRITE
    pdc_access_t access_type;
    // Determine unit size.
    pdc_var_type_t mem_type;
    size_t         unit;
    // User data buffer
    char *buf;
    /* Used internally for 2D and 3D data */
    // Contiguous buffers for read, undefined for PDC_WRITE. Static region mapping has >= 1 number of
    // read_bulk_buf. Other mappings have size of 1.
    char **read_bulk_buf;
    // buffer used for bulk transfer in mercury
    char *new_buf;
    // For each of the contig buffer sent to a server, we have a bulk buffer.
    char **bulk_buf;
    // Reference counter for bulk_buf, if 0, we free it.
    int **                 bulk_buf_ref;
    pdc_region_partition_t region_partition;

    // Consistency semantics required by user
    pdc_consistency_t consistency;

    // Dynamic object partitioning (static region partitioning and dynamic region partitioning)
    int       n_obj_servers;
    uint32_t *obj_servers;
    // Used by static region partitioning, these variables are regions that overlap the static regions of data
    // servers.
    uint64_t **output_offsets;
    uint64_t **sub_offsets;
    uint64_t **output_sizes;
    // Used only when access_type == PDC_WRITE, otherwise it should be NULL.
    char **output_buf;

    // Local region
    int       local_region_ndim;
    uint64_t *local_region_offset;
    uint64_t *local_region_size;
    // Remote region
    int       remote_region_ndim;
    uint64_t *remote_region_offset;
    uint64_t *remote_region_size;
    uint64_t  total_data_size;
    // Object dimensions
    int       obj_ndim;
    uint64_t *obj_dims;
    // Pointer to object info, can be useful sometimes. We do not want to go through PDC ID list many times.
    struct _pdc_obj_info *obj_pointer;
} pdc_transfer_request;

/*********************/
/* Public Prototypes */
/*********************/

/**
 * Region utility functions.
 */
int check_overlap(int ndim, uint64_t *offset1, uint64_t *size1, uint64_t *offset2, uint64_t *size2);

int PDC_region_overlap_detect(int ndim, uint64_t *offset1, uint64_t *size1, uint64_t *offset2,
                              uint64_t *size2, uint64_t **output_offset, uint64_t **output_size);

int memcpy_subregion(int ndim, uint64_t unit, pdc_access_t access_type, char *buf, uint64_t *size,
                     char *sub_buf, uint64_t *sub_offset, uint64_t *sub_size);

int memcpy_overlap_subregion(int ndim, uint64_t unit, char *buf, uint64_t *offset, uint64_t *size, char *buf2,
                             uint64_t *offset2, uint64_t *size2, uint64_t *overlap_offset,
                             uint64_t *overlap_size);

int detect_region_contained(uint64_t *offset, uint64_t *size, uint64_t *offset2, uint64_t *size2, int ndim);

/**
 * \brief Create a region
 *
 * \param ndims [IN]            Number of dimensions
 * \param offset [IN]           Offset of each dimension
 * \param dims_size [IN]        Size of each dimension
 *
 * \return Object id on success/Zero on failure
 */
pdcid_t PDCregion_create(const psize_t ndims, const uint64_t *offset, const uint64_t *dims_size);

/**
 * \brief Close a region
 *
 * \param region_id [IN]        ID of the object
 *
 * \return Non-negative on success/Negative on failure
 */
perr_t PDCregion_close(pdcid_t region_id);

/**
 * ********
 *
 * \param region [IN]           *********
 */
void PDCregion_free(struct pdc_region_info *region);

pdcid_t PDCregion_transfer_create(void *buf, pdc_access_t access_type, pdcid_t obj_id, pdcid_t local_reg,
                                  pdcid_t remote_reg);
/**
 * \brief Start a region transfer from local region to remote region for an object on buf.
 *
 * \param buf [IN]              Start point of an application buffer
 * \param obj_id [IN]           ID of the target object
 * \param data_type [IN]        Data type of data in memory
 * \param local_reg  [IN]       ID of the source region
 * \param remote_reg [IN]       ID of the target region
 *
 * \return Non-negative on success/Negative on failure
 */
perr_t PDCregion_transfer_start(pdcid_t transfer_request_id);

perr_t PDCregion_transfer_start_all(pdcid_t *transfer_request_id, int size);

perr_t PDCregion_transfer_status(pdcid_t transfer_request_id, pdc_transfer_status_t *completed);

perr_t PDCregion_transfer_wait(pdcid_t transfer_request_id);

perr_t PDCregion_transfer_wait_all(pdcid_t *transfer_request_id, int size);

perr_t PDCregion_transfer_close(pdcid_t transfer_request_id);
/**
 * \brief Map an application buffer to an object
 *
 * \param buf [IN]              Start point of an application buffer
 * \param local_type [IN]       Data type of data in memory
 * \param local_reg  [IN]       ID of the source region
 * \param remote_obj [IN]       ID of the target object
 * \param remote_reg [IN]       ID of the target region
 *
 * \return Non-negative on success/Negative on failure
 */
perr_t PDCbuf_obj_map(void *buf, pdc_var_type_t local_type, pdcid_t local_reg, pdcid_t remote_obj,
                      pdcid_t remote_reg);

/**
 * \brief Get region information
 *
 * \param reg_id [IN]           ID of the region
 * \param obj_id [IN]           ID of the object
 *
 * \return Pointer to pdc_region_info struct on success/Null on failure
 */
struct pdc_region_info *PDCregion_get_info(pdcid_t reg_id);

/**
 * \brief Unmap all regions within the object from a buffer (write unmap)
 *
 * \param remote_obj_id [IN]    ID of the target object
 * \param remote_reg_id [IN]    ID of the target region
 *
 * \return Non-negative on success/Negative on failure
 */
perr_t PDCbuf_obj_unmap(pdcid_t remote_obj_id, pdcid_t remote_reg_id);

/**
 * \brief Obtain the region lock
 *
 * \param obj_id [IN]           ID of the object
 * \param reg_id [IN]           ID of the region
 * \param access_type [IN]      Region access type: READ or WRITE
 * \param lock_mode [IN]        Lock mode of the region: BLOCK or NOBLOCK
 *
 * \return Non-negative on success/Negative on failure
 */
perr_t PDCreg_obtain_lock(pdcid_t obj_id, pdcid_t reg_id, pdc_access_t access_type,
                          pdc_lock_mode_t lock_mode);

/**
 * \brief Release the region lock
 *
 * \param obj_id [IN]           ID of the object
 * \param reg_id [IN]           ID of the region
 * \param access_type [IN]      Region access type
 *
 * \return Non-negative on success/Negative on failure
 */
perr_t PDCreg_release_lock(pdcid_t obj_id, pdcid_t reg_id, pdc_access_t access_type);

#endif /* PDC_REGION_H */
