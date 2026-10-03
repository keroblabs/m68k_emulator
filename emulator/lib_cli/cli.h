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

#define EOL_WINDOWS		FALSE
#define EOL_LINUX		TRUE

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

typedef void (*save_print_ptr_t)(void * p_data, const char *s);
typedef char * (*dump_print_ptr_t)(void * p_data, uint8_t index);

typedef bool_t (*api_check_ptr_t)(void * p_data);
typedef char (*api_getc_ptr_t)(void * p_data);
typedef void (*api_putc_ptr_t)(void * p_data, char c);
typedef void (*api_puts_ptr_t)(void * p_data, const char *s);
typedef uint32_t (*api_accept_ptr_t)(uint32_t handler);
typedef uint32_t (*api_prepare_ptr_t)(uint32_t param);
typedef void (*api_close_ptr_t)(void * p_data);

///////////////////////////////////////////////////////////////////////////

typedef struct _cli_cmd_t
{
	char 				cmd[MAX_COMMAND_NAME];	/* Command name.                            */
	cmd_func_ptr_t 		func; 					/* Function pointer to associated function. */
	//void * 				p_data;

	struct _cli_cmd_t* 	p_next;

} cli_command_t;

typedef struct _cli_api_data_t_
{
	api_check_ptr_t 	api_check_func;
	api_getc_ptr_t  	api_getc_func;
	api_putc_ptr_t  	api_putc_func;
	api_puts_ptr_t  	api_puts_func;
	api_accept_ptr_t	api_accept_func;
	api_prepare_ptr_t   api_prepare_func;
	api_close_ptr_t     api_close_func;

} cli_api_data_t;

typedef struct _cli_instance_t_
{
	uint8_t command_buffer[MAX_BUF_SIZE]; 	/* CLI command buffer */
	uint8_t rx_buffer[MAX_BUF_SIZE]; 		/* CLI Rx byte-buffer */
	uint8_t *p_buffer;	   					/* Pointer to Rx byte-buffer */

	bool_t   cmd_pending;
	bool_t   cmd_resend;
	bool_t   echo_enabled;
	bool_t   eol_linux;
	bool_t   pending_close;
	bool_t   prompt_printed;

	uint8_t  esc_index;
	uint16_t esc_delay;

	cli_data_t * cli_parent;

	void * p_comm;
	//void * p_data;
	//void * p_history;

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

typedef struct _cli_env_t_
{
	cli_instance_t * instance_curr;

} cli_env_t;

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

void cli_init(cli_data_t *cli_data);
void cli_command_add(cli_data_t * cli_data, cli_command_t * cli_command, char *cmd, cmd_func_ptr_t func);
void cli_instance_add(cli_data_t * cli_data, cli_instance_t * cli_instance, void * p_iface, bool_t echo);
void cli_instance_remove(cli_instance_t *cli_instance);

void cli_process(cli_instance_t *cli_instance);
int cli_printf(cli_data_t * cli_data, const char *fmt, ...);
void cli_puts(cli_data_t * cli_data, const char *string);

///////////////////////////////////////////////////////////////////////////


#endif 
