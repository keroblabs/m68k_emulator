#include "cli.h"
#include "commands.h"
#include "core_debug.h"
#include "arm_core.h"
#include "core_library.h"

//////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////

static void set_break_func(void * p_cli, int argc, char **argv)
{
    cli_data_t * cli_parent = ( cli_data_t * )p_cli;
    uint32_t address;

    if( argc != 2 )
    {
        cli_printf(cli_parent, "set_break <address>\n\n");
    }
    else
    {
        char * s_arg = argv[1];

        if( !strncmp( s_arg, "0x", 2 ) )
        {
            s_arg += 2;
            sscanf( s_arg, "%x", &address );
        }
        else if( s_arg[ strlen(s_arg) - 1] == 'h' )
        {
            char temp_addr[10];
            strcpy( temp_addr, s_arg );
            temp_addr[ strlen(s_arg) - 1 ] = 0;
            sscanf( temp_addr, "%x", &address );
        }
        else
        {
            cli_printf(cli_parent, "set_break <address>\n\n");
            cli_printf(cli_parent, "Invalid address\n\n");
        }

        uint32_t index = core_debug_add_breakpoint( cli_parent->p_data, address );
        cli_printf(cli_parent, "Add breakpoint %d at 0x%08x\n\n", index, address );
    }
}

static void arm_reset_func(void * p_cli, int argc, char **argv)
{
    cli_data_t * cli_parent = ( cli_data_t * )p_cli;
    core_reset( cli_parent->p_data );
    core_debug_set_run( cli_parent->p_data, FALSE );
	cli_printf(cli_parent, "RESET function executed\n\n");
}

static void arm_run_func(void * p_cli, int argc, char **argv)
{
    cli_data_t * cli_parent = ( cli_data_t * )p_cli;
    core_debug_set_run( cli_parent->p_data, TRUE );
	cli_printf(cli_parent, "RUN function executed\n\n");
}

static void arm_step_func(void * p_cli, int argc, char **argv)
{
    cli_data_t * cli_parent = ( cli_data_t * )p_cli;

    core_debug_set_step( cli_parent->p_data );
	// cli_printf(instance->cli_parent, "Step function executed\n\n");
}

static void arm_show_regs_func(void * p_cli, int argc, char **argv)
{
    cli_data_t * cli_parent = ( cli_data_t * )p_cli;

    core_debug_show_registers( cli_parent->p_data );
	// cli_printf(instance->cli_parent, "Step function executed\n\n");
}

void cli_arm_core_add_commands( void * p_app_data )
{
    app_data_t * app_data = (app_data_t *)p_app_data;

    commands_add_command(app_data, "reset",   arm_reset_func );
    commands_add_command(app_data, "step",    arm_step_func );
    commands_add_command(app_data, "run",     arm_run_func );
    commands_add_command(app_data, "regs",    arm_show_regs_func );
    commands_add_command(app_data, "bkp_add", set_break_func );
}