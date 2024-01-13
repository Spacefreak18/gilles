#ifndef _PARAMETERS_H
#define _PARAMETERS_H

#include <stdbool.h>

#include "../simulatorapi/simapi/simapi/simapi.h"
#include "../simulatorapi/simapi/simapi/simdata.h"
#include "../simulatorapi/simapi/simapi/simmapper.h"

typedef struct
{
    char* sim_string;
    int   program_action;
    int   program_state;
    char* config_path;
    char* gnuplot_file;

    bool  cli;
    bool  mqtt;
    bool  mysql;
    bool  simon;
    int   verbosity_count;
    int err;

    char* db_user;
    char* db_serv;
    char* db_dbnm;
    char* db_pass;
    char* db_conn;

    Simulator sim;
    SimData* simdata;
    SimMap* simmap;
}
Parameters;

typedef enum
{
    A_PLAY          = 0,
    A_BROWSE        = 1,
}
ProgramAction;

typedef enum
{
    E_SUCCESS_AND_EXIT = 0,
    E_SUCCESS_AND_DO   = 1,
    E_SOMETHING_BAD    = 2
}
ConfigError;

typedef enum
{
    E_NO_ERROR         = 0,
    E_BAD_CONFIG       = 1,
    E_FAILED_DB_CONN   = 2,
    E_DB_QUERY_FAIL    = 3,
}
GillesError;

ConfigError getParameters(int argc, char** argv, Parameters* p);
int freeparams(Parameters* p);

struct _errordesc
{
    int  code;
    char* message;
} static errordesc[] =
{
    { E_SUCCESS_AND_EXIT, "No error and exiting" },
    { E_SUCCESS_AND_DO,   "No error and continuing" },
    { E_SOMETHING_BAD,    "Something bad happened" },
};

#endif
