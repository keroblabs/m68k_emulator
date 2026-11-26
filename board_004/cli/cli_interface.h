#ifndef _cli_interface_h_
#define _cli_interface_h_

//////////////////////////////////////////////////////////////////////////////////////

#include "cli.h"
#include "types.h"

//////////////////////////////////////////////////////////////////////////////////////

#define INTERFACE_INVALID           ((uint32_t)-3)
#define INTERFACE_CHECK_FAIL        ((uint32_t)-2)
#define INTERFACE_NOT_READY         ((uint32_t)-1)

//////////////////////////////////////////////////////////////////////////////////////

const cli_api_data_t * cli_interface_get(uint8_t index);

uint32_t cli_interface_pool(uint8_t index, uint32_t iface_hdl);
uint32_t cli_interface_init(uint8_t index, uint32_t param);

void cli_interface_attach(void);

void cli_interface_close(uint8_t index, uint32_t iface_hdl);

//////////////////////////////////////////////////////////////////////////////////////

#endif // _cli_interface_h_
