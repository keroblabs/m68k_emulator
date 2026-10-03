#include "cli.h"
#include "comm_host_interface.h"

///////////////////////////////////////////////////////////////////////////

#define ESC_STATE_NONE		0
#define ESC_STATE_CTRL		1
#define ESC_STATE_CHAR		2
#define ESC_DELAY_COUNT		8192

#define CLI_PRINTF_MAX_SIZE	300

///////////////////////////////////////////////////////////////////////////

static const char cli_newline[] = "\n";
static const char cli_prompt[] = "\n>> "; /* CLI prompt displayed to the user */
static const char cli_unrecog[] = "CMD: Command not recognised\n";

///////////////////////////////////////////////////////////////////////////

static void cli_print(cli_instance_t *cli_instance, const char *msg)
{
	cli_data_t * cli_data = cli_instance->cli_parent;
	comm_interface_buffer_tx(cli_instance->p_comm, (uint8_t *)msg, strlen(msg));
}

static void cli_putc(cli_instance_t *cli_instance, char c)
{
	cli_data_t * cli_data = cli_instance->cli_parent;
	comm_interface_put_tx(cli_instance->p_comm, (uint8_t)c);
}

static void cli_process_pending_command(cli_instance_t *cli_instance)
{
	if (cli_instance->esc_index != ESC_STATE_NONE)
	{
		if ((--cli_instance->esc_delay) == 0)
		{
			if (cli_instance->esc_index == ESC_STATE_CTRL)
			{
				// handle single ESC event
			}
			cli_instance->esc_index = ESC_STATE_NONE;
		}
	}

	if (!cli_instance->cmd_pending)
    {
		return;
    }

	uint8_t argc = 0;
	char *argv[30];

	cli_instance->cmd_resend = FALSE;

	/* Get the first token (cmd name) */
	argv[argc] = strtok(cli_instance->command_buffer, " ");

	/* Walk through the other tokens (parameters) */
	while ((argv[argc] != NULL) && (argc < 30)) 
    {
		argv[++argc] = strtok(NULL, " ");
	}

	if ((argv != NULL) && (argv[0] != NULL))
	{
		/* Search the command table for a matching command, using argv[0]
		* which is the command name. */
		cli_data_t * cli_parent = cli_instance->cli_parent;
		cli_command_t * search_command = cli_parent->command_list;
		while (search_command != NULL)
		{
			if (strcmp(argv[0], search_command->cmd) == 0)
			{
				cli_parent->instance_curr = cli_instance;

				/* Found a match, execute the associated function. */
				search_command->func(cli_parent, argc, argv);
				cli_print(cli_instance, cli_prompt);
				cli_instance->prompt_printed = TRUE;

				cli_parent->instance_curr = NULL;
				cli_instance->cmd_pending = FALSE;
				return;
			}

			search_command = search_command->p_next;
		}

		/* Command not found */
		cli_print(cli_instance, cli_unrecog);
	}

	cli_print(cli_instance, cli_prompt);
	cli_instance->prompt_printed = TRUE;

	cli_instance->cmd_pending = FALSE;
}

static void cli_parse_single_char(cli_instance_t *cli_instance, char c)
{
	char valid_terminador   = '\n';
	char discard_terminator = '\r';
	cli_data_t * cli_parent = cli_instance->cli_parent;

	if (c == valid_terminador)
	{
		if (cli_instance->p_buffer == cli_instance->rx_buffer)
		{
			/* handle ENTER with empty buffer */
			cli_print(cli_instance, "");
			cli_print(cli_instance, cli_prompt);
			cli_instance->prompt_printed = TRUE;
		}
		else if (!cli_instance->cmd_pending) 
		{
			*cli_instance->p_buffer = '\0';
			strcpy(cli_instance->command_buffer, (const char *)cli_instance->rx_buffer);

			if (cli_parent->history_func)
			{
				/* history control is per instance, though the API is global */
				cli_parent->history_func(cli_instance, HISTORY_ADD, cli_instance->rx_buffer, NULL);
			}
			if (cli_instance->echo_enabled)
			{
				cli_print(cli_instance, "\r\n");
			}
			cli_instance->cmd_pending 	 = TRUE;
			cli_instance->prompt_printed = FALSE;
			cli_instance->p_buffer 		 = cli_instance->rx_buffer;
		}
	}
	else if (c == discard_terminator)
	{
	}
	else
	{
		switch (c) 
		{
			case 0x1B:
				cli_instance->esc_index++;
				cli_instance->esc_delay = ESC_DELAY_COUNT;
				break;

			case 0x12:
				/* handler custom command history */
				break;

			case '\b':
				/* Backspace. Delete character. */
				if (cli_instance->p_buffer > cli_instance->rx_buffer)
				{
					cli_instance->p_buffer--;
					cli_putc(cli_instance, 0x08);
					cli_putc(cli_instance, 0x20);
					cli_putc(cli_instance, 0x08);
				}
				break;

			default:
				/* Normal character received, add to buffer. */
				if ((cli_instance->p_buffer - cli_instance->rx_buffer) < MAX_BUF_SIZE)
				{
					*cli_instance->p_buffer++ = c;
					if (cli_instance->echo_enabled == TRUE)
					{
						cli_putc(cli_instance, c);
					}
				}
				break;
		}
	}
}

static void cli_parse_esc_char(cli_instance_t *cli_instance, char c)
{
	cli_data_t * cli_data = cli_instance->cli_parent;

	switch (c) 
	{
		case 'D':
			if (cli_instance->p_buffer > cli_instance->rx_buffer)
			{
				cli_instance->p_buffer--;
				if (cli_instance->echo_enabled == TRUE)
				{
					cli_putc(cli_instance, CMD_BACKSPACE);
				}
			}
			break;			

		case 'C':
			if ((cli_instance->p_buffer - cli_instance->rx_buffer) < MAX_BUF_SIZE)
			{
				*cli_instance->p_buffer++ = ' ';
			}
			break;

		case 'A':
			/* handler command history */
			if (   (cli_instance->cmd_resend == FALSE) 
				&& (cli_data->history_func != NULL) 
				&& (cli_data->history_func(cli_instance, HISTORY_LAST, NULL, cli_instance->rx_buffer) == TRUE))
			{
				cli_instance->p_buffer 		= &cli_instance->rx_buffer[strlen(cli_instance->rx_buffer)];
				cli_instance->cmd_resend 	= TRUE;
				cli_print(cli_instance, cli_instance->rx_buffer);
			}
			break;
			
	}

	cli_instance->esc_index = ESC_STATE_NONE;
}

static void cli_process_check_instance(cli_instance_t *cli_instance)
{
	cli_data_t * cli_data = cli_instance->cli_parent;

	if (comm_interface_check_rx(cli_instance->p_comm) == FALSE)
	{
		return;
	}

	char c = (char)comm_interface_get_rx(cli_instance->p_comm);

	switch (cli_instance->esc_index)
	{
		case ESC_STATE_NONE:
			cli_parse_single_char(cli_instance, c);
			break;

		case ESC_STATE_CTRL:
			if (c == '[') cli_instance->esc_index++;
			else cli_instance->esc_index = ESC_STATE_NONE;
			break;

		case ESC_STATE_CHAR:
			cli_parse_esc_char(cli_instance, c);
			break;
	}
}

void cli_init(cli_data_t *cli_data)
{
	memset(cli_data, 0x00, sizeof(cli_data_t));
}

void cli_instance_add(cli_data_t * cli_data, cli_instance_t * cli_instance, void * p_comm, bool_t echo)
{
	memset(cli_instance, 0x00, sizeof(cli_instance_t));
	
	cli_instance->p_buffer 		= cli_instance->rx_buffer;
	cli_instance->cmd_pending 	= FALSE;
	cli_instance->cmd_resend 	= FALSE;
	cli_instance->echo_enabled  = echo;
	cli_instance->esc_index 	= ESC_STATE_NONE;
	cli_instance->esc_delay 	= 0;
	cli_instance->p_comm		= p_comm;
	cli_instance->p_next 		= cli_data->instance_list;
	cli_instance->cli_parent    = cli_data;

	cli_data->instance_list 	= cli_instance;

	if (cli_data->dump_print_func != NULL)
	{
		uint8_t index = 0;
		char * saved_print;

		while ((saved_print = cli_data->dump_print_func(cli_data, index)) != NULL)
		{
			cli_print(cli_instance, saved_print);
			index++;
		}
	}

	/* Print the CLI prompt. */
	cli_print(cli_instance, cli_prompt);
	cli_instance->prompt_printed = TRUE;
}

void cli_instance_remove(cli_instance_t *cli_instance)
{
	cli_data_t *cli_data = cli_instance->cli_parent;
	cli_instance_t *search_cli = cli_data->instance_list;
	cli_instance_t *prev_cli   = NULL;

	comm_interface_close(cli_instance->p_comm);

	while (search_cli != NULL)
	{
		if (search_cli == cli_instance)
		{
			if (prev_cli != NULL)
			{
				prev_cli->p_next = cli_instance->p_next;
			}
			else
			{
				cli_data->instance_list = cli_instance->p_next;
			}

			break;
		}

		search_cli = search_cli->p_next;
	}
}

void cli_process(cli_instance_t *cli_instance)
{
	cli_process_check_instance(cli_instance);
	cli_process_pending_command(cli_instance);
}

void cli_command_add(cli_data_t * cli_data, cli_command_t * cli_command, char *cmd, cmd_func_ptr_t func)
{
	// If the cmd string is null, we assume the application 
	// filled up the struct, so just link it.
	if (cmd != NULL)
	{
		strcpy(cli_command->cmd, cmd);
		cli_command->func 	= func;
	}

	cli_command->p_next = cli_data->command_list;

	cli_data->command_list = cli_command;
}

void cli_puts(cli_data_t * cli_data, const char *string)
{
	if ((cli_data->instance_curr == NULL) && (cli_data->save_print_func != NULL))
	{
		cli_data->save_print_func(cli_data, string);
	}

	cli_instance_t * search_instance = cli_data->instance_list;

	while (search_instance != NULL)
	{
		if ((cli_data->instance_curr == NULL) || (cli_data->instance_curr == search_instance))
		{
			if (search_instance->prompt_printed == TRUE)
			{
				comm_interface_buffer_tx(search_instance->p_comm, (uint8_t *)cli_newline, strlen(cli_newline));
				search_instance->prompt_printed = FALSE;
			}
			comm_interface_buffer_tx(search_instance->p_comm, (uint8_t *)string, strlen(string));
		}

		search_instance = search_instance->p_next;
	}	
}

int cli_printf(cli_data_t * cli_data, const char *fmt, ...)
{
    va_list args;
    int printed_len;
    static char string[CLI_PRINTF_MAX_SIZE + 1];

    va_start(args, fmt);
    printed_len = vsnprintf(string, CLI_PRINTF_MAX_SIZE, fmt, args);
    va_end(args);

	if ((cli_data->instance_curr == NULL) && (cli_data->save_print_func != NULL))
	{
		cli_data->save_print_func(cli_data, string);
	}

	cli_instance_t * search_instance = cli_data->instance_list;

	while (search_instance != NULL)
	{
		if ((cli_data->instance_curr == NULL) || (cli_data->instance_curr == search_instance))
		{
			if (search_instance->prompt_printed == TRUE)
			{
				comm_interface_buffer_tx(search_instance->p_comm, (uint8_t *)cli_newline, strlen(cli_newline));
				search_instance->prompt_printed = FALSE;
			}
			comm_interface_buffer_tx(search_instance->p_comm, string, strlen(string));
		}

		search_instance = search_instance->p_next;
	}
    
	return printed_len;
}
