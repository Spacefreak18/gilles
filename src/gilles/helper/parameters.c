#include "parameters.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libconfig.h>

#include <argtable2.h>
#include <regex.h>

int freeparams(Parameters* p)
{
    if(p->sim_string != NULL)
    {
        free(p->sim_string);
    }
    if (p->mysql == true)
    {
        if(p->mysql_user != NULL)
        {
            free(p->mysql_user);
        }
    }
    return 0;
}

ConfigError getParameters(int argc, char** argv, Parameters* p)
{

    ConfigError exitcode = E_SOMETHING_BAD;

    // set return structure defaults
    p->program_action      = 0;
    p->cli                 = false;
    p->mqtt                = false;
    p->mysql               = false;
    p->simon               = false;
    p->verbosity_count     = 0;

    // setup argument handling structures
    const char* progname = "Gilles";

    struct arg_lit* arg_verbosity   = arg_litn("v","verbose", 0, 2, "increase logging verbosity");

    struct arg_rex* cmd1             = arg_rex1(NULL, NULL, "play", NULL, REG_ICASE, NULL);
    struct arg_str* arg_sim          = arg_strn("s", "sim", "<gamename>", 0, 1, NULL);
    struct arg_lit* arg_cli          = arg_lit0("c",  "textui", "text only ui");
    struct arg_lit* arg_mqtt         = arg_lit0("Q",  "mqtt", "send data to local mqtt server with connection settings speciifed in config");
    struct arg_lit* arg_mysql        = arg_lit0("M",  "mysql", "send data to local mysql server with connection settings specified in config");
    struct arg_lit* help             = arg_litn(NULL,"help", 0, 1, "print this help and exit");
    struct arg_lit* vers             = arg_litn(NULL,"version", 0, 1, "print version information and exit");
    struct arg_end* end              = arg_end(20);
    void* argtable[]                 = {cmd1,arg_sim,arg_verbosity,arg_cli,arg_mqtt,arg_mysql,help,vers,end};
    int nerrors;



    if (arg_nullcheck(argtable) != 0)
    {
        printf("%s: insufficient memory\n",progname);
        goto cleanup;
    }

    nerrors = arg_parse(argc,argv,argtable);

    if (nerrors==0)
    {
        p->program_action = A_PLAY;
        p->sim_string = strdup(arg_sim->sval[0]);
        p->verbosity_count = arg_verbosity->count;
        if (arg_cli->count > 0)
        {
            p->cli = true;
        }
        if (arg_mqtt->count > 0)
        {
            p->mqtt = true;
        }
        if (arg_mysql->count > 0)
        {
            p->mysql = true;
        }
        exitcode = E_SUCCESS_AND_DO;
    }

    // interpret some special cases before we go through trouble of reading the config file
    if (help->count > 0)
    {
        printf("Usage: %s\n", progname);
        arg_print_syntax(stdout,argtable,"\n");
        printf("\nReport bugs on the github github.com/spacefreak18/gilles.\n");
        exitcode = E_SUCCESS_AND_EXIT;
        goto cleanup;
    }

    if (vers->count > 0)
    {
        printf("%s Simulator Monitor\n",progname);
        printf("October 2022, Paul Dino Jones\n");
        exitcode = E_SUCCESS_AND_EXIT;
        goto cleanup;
    }

cleanup:
    arg_freetable(argtable,sizeof(argtable)/sizeof(argtable[0]));
    return exitcode;

}
