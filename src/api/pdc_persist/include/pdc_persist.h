#ifndef PDC_PERSIST_H
#define PDC_PERSIST_H

#include "pdc_private.h"
#include "pdc_public.h"
#include "pdc_obj.h"

perr_t pdcTransferStart(pdcid_t transfer_request_id);
perr_t pdcTransferWait(pdcid_t transfer_request_id);

void wait_my_rpc();

#endif /* PDC_PERSIST_H */
