#ifndef PDC_PERSIST_H
#define PDC_PERSIST_H

typedef struct PDCtransfer_st *pdcTransfer_t;

void pdcTransferCreate(pdcTransfer_t* pTransfer);
void pdcTransferStart(pdcTransfer_t pTransfer);
void pdcTransferWait(pdcTransfer_t pTransfer);

void wait_my_rpc();

#endif /* PDC_PERSIST_H */
