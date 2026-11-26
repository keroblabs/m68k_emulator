#ifndef _CLI_DEFS_H_
#define _CLI_DEFS_H_

///////////////////////////////////////////////////////////////////////////

#include "types.h"

///////////////////////////////////////////////////////////////////////////

#ifndef MAX_BUF_SIZE
#define MAX_BUF_SIZE 128 /* Maximum size of CLI Rx buffer */
#endif

#ifndef CMD_TERMINATOR_VALID
#define CMD_TERMINATOR_VALID '\r' 	/* Delimiter denoting end of cmd from user */
#endif

#ifndef CMD_TERMINATOR_DISCARD
#define CMD_TERMINATOR_DISCARD '\n' /* Delimiter ignored by this system */
#endif

#ifndef CMD_BACKSPACE
#define CMD_BACKSPACE	0x08
#endif

#ifndef MAX_COMMAND_NAME
#define MAX_COMMAND_NAME 20
#endif

#define ECHO_ENABLED	TRUE
#define ECHO_DISABLED	FALSE

///////////////////////////////////////////////////////////////////////////

enum
{
	HISTORY_ADD,
    HISTORY_LAST,
    HISTORY_DISPLAY,
    HISTORY_SEARCH,
};

///////////////////////////////////////////////////////////////////////////

typedef struct _cli_cmd_t 		cli_command_t;
typedef struct _cli_instance_t_ cli_instance_t;
typedef struct _cli_t_ 			cli_data_t;

///////////////////////////////////////////////////////////////////////////

typedef void (*cmd_func_ptr_t)(void * p_data, int argc, char **argv);
typedef bool_t (*history_func_ptr_t)(void *p_data, uint8_t command, char * input, char * output);

typedef void (*save_print_ptr_t)(void * p_data, char *s);
typedef char * (*dump_print_ptr_t)(void * p_data, uint8_t index);

typedef bool_t (*api_check_ptr_t)(void * p_data);
typedef char (*api_getc_ptr_t)(void * p_data);
typedef void (*api_putc_ptr_t)(void * p_data, char c);

typedef void (*api_puts_ptr_t)(void * p_data, const char *s);
typedef uint16_t (*api_even_func_t)(void * p_data, char * keycode);

///////////////////////////////////////////////////////////////////////////

typedef struct _cli_cmd_t
{
	char 				cmd[MAX_COMMAND_NAME];	/* Command name.                            */
	cmd_func_ptr_t 		func; 					/* Function pointer to associated function. */
	void * 				p_data;

	struct _cli_cmd_t* 	p_next;

} cli_command_t;

typedef struct _cli_api_data_t_
{
	api_puts_ptr_t  	api_puts_func;
	api_even_func_t     event_func;

} cli_api_data_t;

typedef struct _cli_instance_t_
{
	char command_buffer[MAX_BUF_SIZE]; 	/* CLI command buffer */
	char rx_buffer[MAX_BUF_SIZE]; 		/* CLI Rx byte-buffer */
	char *p_buffer;	   					/* Pointer to Rx byte-buffer */

	bool_t   cmd_pending;
	bool_t   cmd_resend;
	bool_t   echo_enabled;

	cli_data_t * cli_parent;

	const cli_api_data_t * cli_api_io;

	void * p_data;

	struct _cli_instance_t_ * p_next;

} cli_instance_t;

typedef struct _cli_t_
{
	save_print_ptr_t	save_print_func;
	dump_print_ptr_t	dump_print_func;
	history_func_ptr_t 	history_func;

	cli_instance_t * instance_curr;
	cli_instance_t * instance_list;
	cli_command_t * command_list;

	void * p_data;

} cli_data_t;

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

void cli_init(cli_data_t *cli_data);
void cli_command_add(cli_data_t * cli_data, cli_command_t * cli_command, char *cmd, cmd_func_ptr_t func, void * p_data);
void cli_instance_add(cli_data_t * cli_data, cli_instance_t * cli_instance, const cli_api_data_t * io_api, void * p_data, bool_t echo);

void cli_process(cli_data_t *cli_data);
int cli_printf(cli_data_t * cli_data, const char *fmt, ...);

///////////////////////////////////////////////////////////////////////////


#endif //
