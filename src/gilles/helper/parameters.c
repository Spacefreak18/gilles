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
    if (p->mysql == true || p->program_action == A_BROWSE)
    {
        if(p->db_user != NULL)
        {
            free(p->db_user);
        }
        if(p->db_pass != NULL)
        {
            free(p->db_pass);
        }
        if(p->db_serv != NULL)
        {
            free(p->db_serv);
        }
        if(p->db_dbnm != NULL)
        {
            free(p->db_dbnm);
        }
        if(p->db_conn != NULL)
        {
            free(p->db_conn);
        }
        if(p->gnuplot_file != NULL)
        {
            free(p->gnuplot_file);
        }
        if(p->gnuplot_bin != NULL)
        {
            free(p->gnuplot_bin);
        }
    }
    if (p->config_path != NULL)
    {
        free(p->config_path);
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

    p->config_path         = NULL;
    p->sim_string          = NULL;
    p->db_user             = NULL;
    p->db_pass             = NULL;
    p->db_serv             = NULL;
    p->db_dbnm             = NULL;
    p->db_conn             = NULL;
    p->gnuplot_file        = NULL;
    p->gnuplot_bin         = NULL;

    // setup argument handling structures
    const char* progname = "Gilles";

    struct arg_lit* arg_verbosity0   = arg_litn("v","verbose", 0, 2, "increase logging verbosity");
    struct arg_lit* arg_verbosity1   = arg_litn("v","verbose", 0, 2, "increase logging verbosity");

    struct arg_rex* cmd1             = arg_rex1(NULL, NULL, "play", NULL, REG_ICASE, NULL);
    struct arg_rex* cmd2             = arg_rex1(NULL, NULL, "browse", NULL, REG_ICASE, NULL);
    struct arg_str* arg_cpath        = arg_strn("f", "configpath", "<path of config>", 0, 1, NULL);
    struct arg_str* arg_sim          = arg_strn("s", "sim", "<gamename>", 0, 1, NULL);
    struct arg_lit* arg_cli          = arg_lit0("c",  "textui", "text only ui");
    struct arg_lit* arg_mqtt         = arg_lit0("Q",  "mqtt", "send data to local mqtt server with connection settings speciifed in config");
    struct arg_lit* arg_mysql        = arg_lit0("M",  "mysql", "send data to local mysql server with connection settings specified in config");
    struct arg_lit* help0             = arg_litn(NULL,"help", 0, 1, "print this help and exit");
    struct arg_lit* vers0             = arg_litn(NULL,"version", 0, 1, "print version information and exit");
    struct arg_end* end0              = arg_end(20);
    struct arg_lit* help1             = arg_litn(NULL,"help", 0, 1, "print this help and exit");
    struct arg_lit* vers1             = arg_litn(NULL,"version", 0, 1, "print version information and exit");
    struct arg_end* end1              = arg_end(20);
    void* argtable0[]                 = {cmd1,arg_sim,arg_verbosity0,arg_cli,arg_mqtt,arg_mysql,help0,vers0,end0};
    void* argtable1[]                 = {cmd2,arg_verbosity1,help1,vers1,end1};
    int nerrors0;
    int nerrors1;



    if (arg_nullcheck(argtable0) != 0)
    {
        printf("%s: insufficient memory\n",progname);
        goto cleanup;
    }
    if (arg_nullcheck(argtable1) != 0)
    {
        printf("%s: insufficient memory\n",progname);
        goto cleanup;
    }

    nerrors0 = arg_parse(argc,argv,argtable0);
    nerrors1 = arg_parse(argc,argv,argtable1);

    if (nerrors0==0)
    {
        p->program_action = A_PLAY;
        p->sim_string = strdup(arg_sim->sval[0]);
        p->verbosity_count = arg_verbosity0->count;
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
        if (arg_cpath->sval[0] != NULL)
        {
            p->config_path = strdup(arg_cpath->sval[0]);
        }
        exitcode = E_SUCCESS_AND_DO;
    }
    else
    {
        if (nerrors1==0)
        {
            p->program_action = A_BROWSE;
            //p->sim_string = strdup(arg_sim->sval[0]);
            p->verbosity_count = arg_verbosity1->count;
            exitcode = E_SUCCESS_AND_DO;
        }
    }
    // interpret some special cases before we go through trouble of reading the config file
    if (help0->count > 0)
    {
        printf("Usage: %s\n", progname);
        arg_print_syntax(stdout,argtable0,"\n");
        arg_print_syntax(stdout,argtable1,"\n");
        printf("\nReport bugs on the github github.com/spacefreak18/gilles.\n");
        exitcode = E_SUCCESS_AND_EXIT;
        goto cleanup;
    }

    if (vers0->count > 0)
    {
        printf("%s Simulator Monitor\n",progname);
        printf("October 2022, Paul Dino Jones\n");
        exitcode = E_SUCCESS_AND_EXIT;
        goto cleanup;
    }

cleanup:
    arg_freetable(argtable0,sizeof(argtable0)/sizeof(argtable0[0]));
    arg_freetable(argtable1,sizeof(argtable1)/sizeof(argtable1[0]));
    return exitcode;

}
