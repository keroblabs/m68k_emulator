#include "cli.h"
#include "events_enum.h"
#include "debug.h"

///////////////////////////////////////////////////////////////////////////

#define ESC_STATE_NONE		0
#define ESC_STATE_CTRL		1
#define ESC_STATE_CHAR		2
#define ESC_DELAY_COUNT		8192

#define CLI_PRINTF_MAX_SIZE	200

///////////////////////////////////////////////////////////////////////////

const char cli_prompt[] = "\n>> "; /* CLI prompt displayed to the user */
const char cli_unrecog[] = "CMD: Command not recognised\n";

///////////////////////////////////////////////////////////////////////////

static void cli_print(cli_instance_t *cli_instance, const char *msg)
{
	//cli_data_t * cli_data = cli_instance->cli_parent;
	cli_instance->cli_api_io->api_puts_func(cli_instance->p_data, msg);
}

static void cli_putc(cli_instance_t *cli_instance, char c)
{
	//cli_data_t * cli_data = cli_instance->cli_parent;
	char msg[2];
	msg[0] = c;
	msg[1] = 0;
	cli_instance->cli_api_io->api_puts_func(cli_instance->p_data, msg);
}

static void cli_process_pending_command(cli_instance_t *cli_instance)
{
	if(!cli_instance->cmd_pending)
    {
		return;
    }

	uint8_t argc = 0;
	char *argv[30];
	char *saved_ptr;

	cli_instance->cmd_resend = FALSE;

	/* Get the first token (cmd name) */
	argv[argc] = strtok_r(cli_instance->command_buffer, " ", &saved_ptr);

	/* Walk through the other tokens (parameters) */
	while((argv[argc] != NULL) && (argc < 30)) 
    {
		argv[++argc] = strtok_r(NULL, " ", &saved_ptr);
	}

	if(argv[0] != NULL)
	{
		/* Search the command table for a matching command, using argv[0]
		* which is the command name. */
		cli_command_t * search_command = cli_instance->cli_parent->command_list;
		while(search_command != NULL)
		{
			if ((search_command->cmd[0] == 0) ||
				(strcmp(argv[0], search_command->cmd) == 0))
			{
				cli_instance->cli_parent->instance_curr = cli_instance;

				/* Found a match, execute the associated function. */
				search_command->func(search_command->p_data, argc, argv);
				cli_print(cli_instance, cli_prompt);

				cli_instance->cli_parent->instance_curr = NULL;
				cli_instance->cmd_pending = FALSE;
				return;
			}

			search_command = search_command->p_next;
		}

		/* Command not found */
		cli_print(cli_instance, cli_unrecog);
	}

	cli_print(cli_instance, cli_prompt);

	cli_instance->cmd_pending = FALSE;
}

static void cli_process_event_internal(cli_instance_t *cli_instance)
{
	if(cli_instance->cli_api_io->event_func != NULL)
	{
		char c;
		uint16_t message = cli_instance->cli_api_io->event_func(cli_instance->p_data, &c);
		cli_data_t * cli_data = cli_instance->cli_parent;

		switch(message)
		{
			case INPUT_EVENT_KEY_ESC:
			    // handle single ESC event
				break;

			case INPUT_EVENT_KEY_ENTER:
				if(cli_instance->p_buffer == cli_instance->rx_buffer)
				{
					/* handle ENTER with empty buffer */
					cli_print(cli_instance, "");
					cli_print(cli_instance, cli_prompt);
				}
				else if(!cli_instance->cmd_pending) 
				{
					*cli_instance->p_buffer = '\0';
					strcpy(cli_instance->command_buffer, (const char * )cli_instance->rx_buffer);

					if(cli_instance->cli_parent->history_func)
					{
						/* history control is per instance, though the API is global */
						cli_instance->cli_parent->history_func(cli_instance->p_data, HISTORY_ADD, cli_instance->rx_buffer, NULL);
					}
					if(cli_instance->echo_enabled)
					{
						cli_putc(cli_instance, '\n');
					}
					cli_instance->cmd_pending 	= TRUE;
					cli_instance->p_buffer 		= cli_instance->rx_buffer;
				}
				break;

			case INPUT_EVENT_HISTORY:
				/* handler custom command history */
				break;

			case INPUT_EVENT_KEY_BACKSPACE:
				/* Backspace. Delete character. */
				if(cli_instance->p_buffer > cli_instance->rx_buffer)
				{
					cli_instance->p_buffer--;
					if(cli_instance->echo_enabled == TRUE)
					{
						cli_putc(cli_instance, CMD_BACKSPACE);
					}
				}
				break;

			case INPUT_EVENT_KEY_LEFT:
				if(cli_instance->p_buffer > cli_instance->rx_buffer)
				{
					cli_instance->p_buffer--;
					if(cli_instance->echo_enabled == TRUE)
					{
						cli_print(cli_instance, "\x1b[D");
					}
				}
				break;			

			case INPUT_EVENT_KEY_RIGHT:
				if (*cli_instance->p_buffer != 0)
				//if((cli_instance->p_buffer - cli_instance->rx_buffer) < MAX_BUF_SIZE)
				{
					cli_instance->p_buffer++;
					cli_print(cli_instance, "\x1b[C");
				}
				break;

			case INPUT_EVENT_KEY_UP:
				/* handler command history */
				if(	   (cli_instance->cmd_resend == FALSE) 
					&& (cli_data->history_func != NULL) 
					&& (cli_data->history_func(cli_instance->p_data, HISTORY_LAST, NULL, cli_instance->rx_buffer) == TRUE))
				{
					cli_instance->p_buffer 		= &cli_instance->rx_buffer[strlen(cli_instance->rx_buffer)];
					cli_instance->cmd_resend 	= TRUE;
					cli_print(cli_instance, cli_instance->rx_buffer);
				}
				break;

			case INPUT_EVENT_KEYPRESS:
				/* Normal character received, add to buffer. */
				if((cli_instance->p_buffer - cli_instance->rx_buffer) < MAX_BUF_SIZE)
				{
					*cli_instance->p_buffer++ = c;
					if(cli_instance->echo_enabled == TRUE)
					{
						cli_putc(cli_instance, c);
					}
				}
				break;
		}
	}
}

void cli_init(cli_data_t *cli_data)
{
	memset(cli_data, 0x00, sizeof(cli_data_t));
}

void cli_instance_add(cli_data_t * cli_data, cli_instance_t * cli_instance, const cli_api_data_t * io_api, void * p_data, bool_t echo)
{
	memset(cli_instance, 0x00, sizeof(cli_instance_t));

	//debug_printf( "++ 0x%08x\n", cli_instance );
	//debug_printf( "++ 0x%08x\n", cli_data->instance_list );
	
	cli_instance->p_buffer 		= cli_instance->rx_buffer;
	cli_instance->cmd_pending 	= FALSE;
	cli_instance->cmd_resend 	= FALSE;
	cli_instance->echo_enabled  = echo;
	cli_instance->p_data		= p_data;
	cli_instance->p_next 		= cli_data->instance_list;
	cli_instance->cli_parent    = cli_data;
	cli_instance->cli_api_io	= io_api;

	cli_data->instance_list 	= cli_instance;

	if(cli_data->dump_print_func != NULL)
	{
		uint8_t index = 0;
		char * saved_print;

		while((saved_print = cli_data->dump_print_func(cli_data, index)) != NULL)
		{
			cli_print(cli_instance, saved_print);
			index++;
		}
	}

	/* Print the CLI prompt. */
	cli_print(cli_instance, cli_prompt);
}

void cli_process(cli_data_t *cli_data)
{
	cli_instance_t * search_instance = cli_data->instance_list;

	while(search_instance != NULL)
	{
		cli_process_event_internal(search_instance);
		cli_process_pending_command(search_instance);

		search_instance = search_instance->p_next;
	}
}

void cli_command_add(cli_data_t * cli_data, cli_command_t * cli_command, char *cmd, cmd_func_ptr_t func, void * p_data )
{
	if (cmd == NULL) cli_command->cmd[0] = 0;
	else strcpy( cli_command->cmd, cmd );

	cli_command->func 	= func;
	cli_command->p_data = p_data;

	cli_command->p_next = cli_data->command_list;

	cli_data->command_list = cli_command;
}

int cli_printf(cli_data_t * cli_data, const char *fmt, ...)
{
    va_list args;
    int printed_len;
    char string[CLI_PRINTF_MAX_SIZE + 1];

    va_start(args, fmt);
    printed_len = vsnprintf(string, CLI_PRINTF_MAX_SIZE, fmt, args);
    va_end(args);
	int cnt = 0;

	if((cli_data->instance_curr == NULL) && (cli_data->save_print_func != NULL))
	{
		cli_data->save_print_func(cli_data, string);
	}

	cli_instance_t * search_instance = cli_data->instance_list;

	while(search_instance != NULL)
	{
		//debug_printf( ">> %d 0x%08x\n", cnt++, search_instance );
		if((cli_data->instance_curr == NULL) || (cli_data->instance_curr == search_instance))
		{
			search_instance->cli_api_io->api_puts_func(search_instance->p_data, string);
		}

		search_instance = search_instance->p_next;
	}
    
	return printed_len;
}
