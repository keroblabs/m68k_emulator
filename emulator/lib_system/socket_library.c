#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include "socket_library.h"

///////////////////////////////////////////////////////////////////////////

static WORD     wVersionRequested;
static WSADATA  wsaData;
static bool_t   is_connected = FALSE;

///////////////////////////////////////////////////////////////////////////

void socket_library_init( void )
{
	int		err;
    
	wVersionRequested = MAKEWORD( 1, 1 );
	err = WSAStartup( wVersionRequested, &wsaData );

	if ( err != 0 ) 
	{
		printf("WSAStartup error %ld", WSAGetLastError() );
		WSACleanup();
	}    
}

void socket_close_port( uint32_t socket )
{
    shutdown(socket, SD_BOTH);
    closesocket( socket );    
}

uint32_t socket_listen_port( uint16_t port )
{
    struct	    sockaddr_in channel;
    int		    b, l, on;
    u_long      iMode = 1;
    SOCKET      _l_socket;
    
	//---- Build address structure to bind to socket.--------  
	memset(&channel, 0, sizeof(channel));// zerochannel 
	channel.sin_family = AF_INET; 
	channel.sin_addr.s_addr = htonl(INADDR_ANY); 
	channel.sin_port = htons( port ); 
    
	// ---- create SOCKET--------------------------------------
	_l_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);    
	if (_l_socket < 0) 
	{
		printf("socket error %ld",WSAGetLastError() );
		WSACleanup();
		return ( uint32_t )SOCKET_FAIL_LISTEN;
	}

	on = 1;
	setsockopt(_l_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on)); 
    ioctlsocket(_l_socket, FIONBIO, &iMode);
    
	//---------------------------------------------------------

	//---- BIND socket ----------------------------------------
	b = bind(_l_socket, (struct sockaddr *) &channel, sizeof(channel)); 
	if (b < 0) 
	{
		printf("bind error %ld", WSAGetLastError() ); 
		WSACleanup();
		return ( uint32_t )SOCKET_FAIL_LISTEN;
	}
	//----------------------------------------------------------

	//---- LISTEN socket ----------------------------------------
	l = listen(_l_socket, QUEUE_SIZE);                 // specify queue size 
	if (l < 0) 
	{
		printf("listen error %ld",WSAGetLastError() );
		WSACleanup();
        return ( uint32_t )SOCKET_FAIL_LISTEN;
	}
    
    return ( uint32_t )_l_socket;
}

uint32_t socket_accept_port( uint32_t socket )
{
    SOCKET sa;
    
    sa = accept(socket, NULL, NULL);                  // block for connection request  
    if ((int)sa < 0)
    {
        // 10035 = no connection accepted yet
        
        if(WSAGetLastError() != 10035)
        {
            // printf("accept error %ld ", WSAGetLastError() ); 
            WSACleanup();
            return SOCKET_FAIL_LISTEN;
        }
        
        // not connected yet
        return SOCKET_NOT_READY;
    }

    is_connected = TRUE;
    return ( uint32_t )sa;
}

uint32_t socket_connect_addr( uint8_t * ip_addr, uint16_t port )
{
    SOCKET      sa;
    struct	    sockaddr_in channel;
    int		    b;
    
	//---- Build address structure to bind to socket.--------  
	memset(&channel, 0, sizeof(channel));// zerochannel 
	channel.sin_family = AF_INET; 
	channel.sin_addr.s_net = ip_addr[0];
	channel.sin_addr.s_host = ip_addr[1];
	channel.sin_addr.s_lh = ip_addr[2];
	channel.sin_addr.s_impno = ip_addr[3];
    // channel.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	channel.sin_port = htons( port ); 
    
    sa = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if( sa == INVALID_SOCKET )
    {
		printf("socket error %ld",WSAGetLastError() );
		WSACleanup();
		return 0;        
    }
    
    b = connect( sa, (struct sockaddr *) &channel, sizeof(channel));
	if (b == SOCKET_ERROR) 
	{
		printf("connect error %ld", WSAGetLastError() ); 
		WSACleanup();
        closesocket( sa );
		return 0;
	}    
    
    return ( uint32_t )sa;
}

int socket_read_data( uint32_t socket, uint8_t *buffer, uint32_t max_size )
{
    u_long num_bytes_present = 0;

    ioctlsocket( socket, FIONREAD ,&num_bytes_present);
    
    if( ( num_bytes_present > 0 ) && ( buffer != NULL ) )
    {
        if( num_bytes_present > max_size ) num_bytes_present = max_size;
		recv( socket, (char*)buffer, num_bytes_present, 0 );
    }
    
    return ( int )num_bytes_present;
}

int socket_write_data( uint32_t socket, uint8_t *buffer, uint32_t size )
{
    size = send( socket, (char*)buffer, size, 0 );
    //printf( "OUT %d\r\n", size );
    return (int)size;    
}
