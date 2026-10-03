#ifndef _rwnx_ipc_host_interface_h_
#define _rwnx_ipc_host_interface_h_

//////////////////////////////////////////////////////////////////////////////////////

#include "types.h"

//////////////////////////////////////////////////////////////////////////////////////

#define INTERFACE_INVALID           ((uint32_t)-3)
#define INTERFACE_CHECK_FAIL        ((uint32_t)-2)
#define INTERFACE_NOT_READY         ((uint32_t)-1)

//////////////////////////////////////////////////////////////////////////////////////

void * comm_interface_pool(void * p_iface);

bool_t comm_interface_check_rx(void * p_data);

uint16_t comm_interface_rx_pending(void * p_data);

uint8_t comm_interface_get_rx(void * p_data);

uint16_t comm_interface_buffer_rx(void * p_data, uint8_t * buffer, uint16_t size);

void comm_interface_put_tx(void * p_data, uint8_t c);

void comm_interface_buffer_tx(void * p_data, uint8_t * buffer, uint16_t size);
void comm_interface_close(void * p_data);

void * comm_interface_open(uint32_t param);
void * comm_interface_connect(uint8_t * ip_addr, uint32_t port);

void comm_interface_init(void);

//////////////////////////////////////////////////////////////////////////////////////

#endif // _rwnx_ipc_host_interface_h_
