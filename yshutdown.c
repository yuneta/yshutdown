/****************************************************************************
 *          YSHUTDOWN.C
 *
 *          Shutdown all Yuneta processes, including the agent
 *
 *          Copyright (c) 2016 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/
#include <stdio.h>
#include <argp.h>
#include <errno.h>
#include <regex.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <yuneta.h>

/***************************************************************************
 *              Constants
 ***************************************************************************/
#define NAME        "yshutdown"
#define DOC         "Shutdown all Yuneta processes, including the agent"

#define APP_VERSION     "4.19.0"
#define APP_DATETIME    __DATE__ " " __TIME__
#define APP_SUPPORT     "<niyamaka at yuneta.io>"

/***************************************************************************
 *              Structures
 ***************************************************************************/
/*
 *  Used by main to communicate with parse_opt.
 */
#define MIN_ARGS 0
#define MAX_ARGS 0
struct arguments
{
    char *args[MAX_ARGS+1];     /* positional args */
    int no_kill_agent;
    int verbose;
};

/***************************************************************************
 *              Prototypes
 ***************************************************************************/
static error_t parse_opt (int key, char *arg, struct argp_state *state);

/***************************************************************************
 *      Data
 ***************************************************************************/
const char *argp_program_version = NAME " " APP_VERSION;
const char *argp_program_bug_address = APP_SUPPORT;

/* Program documentation. */
static char doc[] = DOC;

/* A description of the arguments we accept. */
static char args_doc[] = "";

/*
 *  The options we understand.
 *  See https://www.gnu.org/software/libc/manual/html_node/Argp-Option-Vectors.html
 */
static struct argp_option options[] = {
/*-name-------------key-----arg---------flags---doc-----------------group */
{"verbose",         'l',    0,          0,      "Verbose mode."},
{"no-kill-agent",   'n',    0,          0,      " Don't kill Yuneta agent."},
{0}
};

/* Our argp parser. */
static struct argp argp = {
    options,
    parse_opt,
    args_doc,
    doc
};

/***************************************************************************
 *  Parse a single option
 ***************************************************************************/
static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
    /*
     *  Get the input argument from argp_parse,
     *  which we know is a pointer to our arguments structure.
     */
    struct arguments *arguments = state->input;

    switch (key) {
    case 'n':
        arguments->no_kill_agent = 1;
        break;

    case 'l':
        arguments->verbose = 1;
        break;

    case ARGP_KEY_ARG:
        if (state->arg_num >= MAX_ARGS) {
            /* Too many arguments. */
            argp_usage (state);
        }
        arguments->args[state->arg_num] = arg;
        break;

    case ARGP_KEY_END:
        if (state->arg_num < MIN_ARGS) {
            /* Not enough arguments. */
            argp_usage (state);
        }
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
int kill_yuno(const char *directory, const char *pidfile, int verbose)
{
    int pid = 0;
    FILE *file = fopen(pidfile, "r");
    if(!file) {
        return -1;
    }
    fscanf(file, "%d", &pid);
    fclose(file);

    int ret = kill(pid, SIGKILL);
    if(ret == 0) {
        unlink(pidfile);
        if(verbose) {
            printf("Pid %d, killed ('%s')\n", pid, pidfile);
        }
    } else if(errno == ESRCH) {
        unlink(pidfile);
    } else {
        printf("Pid %d, cannot kill ('%s'). Error '%s'\n", pid, pidfile, strerror(errno));
        return -1;
    }
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
BOOL find_yuno_pid_cb(
    void *user_data,
    wd_found_type type,     // type found
    char *fullpath,         // directory+filename found
    const char *directory,  // directory of found filename
    char *name,             // dname[255]
    int level,              // level of tree where file found
    int index               // index of file inside of directory, relative to 0
)
{
    int verbose = (int)(size_t)user_data;
    kill_yuno(directory, fullpath, verbose);
    return TRUE; // to continue
}

int shutdown_yuneta(int no_kill_agent, int verbose)
{
    walk_dir_tree(
        "/yuneta/realms",
        "yuno.pid",
        WD_RECURSIVE|WD_MATCH_REGULAR_FILE,
        find_yuno_pid_cb,
        (void *)(size_t)verbose
    );
    if(!no_kill_agent) {
        kill_yuno("/yuneta/agent", "/yuneta/realms/agent/yuneta_agent.pid", verbose);
    }
    return 0;
}

/***************************************************************************
 *                      Main
 ***************************************************************************/
int main(int argc, char *argv[])
{
    struct arguments arguments;

    /*
     *  Default values
     */
    arguments.no_kill_agent = 0;
    arguments.verbose = 0;

    /*
     *  Parse arguments
     */
    argp_parse (&argp, argc, argv, 0, 0, &arguments);

    /*
     *  Do your work
     */
    return shutdown_yuneta(arguments.no_kill_agent, arguments.verbose);
}
