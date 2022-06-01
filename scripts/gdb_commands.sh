
# break pdc_server_region_request_handler.h:transfer_request_bulk_transfer_write_cb
# p local_bulk_args->in

break pdc_server_region_request_handler.h:transfer_request_cb
r
n 20
p in

######################################

# break obj_transformation.c:236

# break obj_transformation.c:350

# break obj_transformation.c:319

break pdc_client_connect.c:PDC_Client_transfer_request
r

p region_id1
p region_id11
p obj_id11

