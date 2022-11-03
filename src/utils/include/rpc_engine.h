/**
 * @file
 * @author John J. Ravi (jjravi)
 *
 * @brief API of generic utilities and progress engine hooks
 *        that are reused across many RPC functions.
 */

#ifndef RPC_ENGINE_H
#define RPC_ENGINE_H

#include <mercury.h>
#include <mercury_bulk.h>
#include <mercury_macros.h>

void hg_engine_init(hg_bool_t listen, const char *local_addr);
void hg_engine_finalize();
hg_class_t *hg_engine_get_class();
void hg_engine_save_self_addr();
void hg_engine_addr_lookup(const char *name, hg_addr_t *addr);
void hg_engine_addr_free(hg_addr_t addr);
void hg_engine_create_handle(hg_addr_t addr, hg_id_t id, hg_handle_t *handle);

#endif // RPC_ENGINE_H
