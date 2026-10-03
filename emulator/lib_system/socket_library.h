#ifndef _SOCKET_LIBRARY_H_
#define _SOCKET_LIBRARY_H_

//////////////////////////////////////////////////////////////////////////////////////

#include "types.h"

//////////////////////////////////////////////////////////////////////////////////////

#define MIN_SOCKET_RESP_SIZE    25
#define SOCKET_ERROR_TOKEN		"ERR"

#define MAX_SOCKET              20
#define QUEUE_SIZE              10

#define SOCKET_ACCEPTED         ( ( uint32_t )-4 )
#define SOCKET_FAIL_OPEN        ( ( uint32_t )-3 )
#define SOCKET_NOT_READY        ( ( uint32_t )-2 )
#define SOCKET_FAIL_LISTEN      ( ( uint32_t )-1 )

//////////////////////////////////////////////////////////////////////////////////////

void socket_library_init( void );

uint32_t socket_accept_port( uint32_t socket );
uint32_t socket_connect_addr( uint8_t * ip_addr, uint16_t port );
uint32_t socket_listen_port( uint16_t port );

int socket_read_data( uint32_t socket, uint8_t *buffer, uint32_t max_size );
int socket_write_data( uint32_t socket, uint8_t *buffer, uint32_t size );

void socket_close_port( uint32_t socket );

//////////////////////////////////////////////////////////////////////////////////////

#endif // _SOCKET_LIBRARY_H_
