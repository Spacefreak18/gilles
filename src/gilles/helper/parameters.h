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

    bool  cli;
    bool  mqtt;
    bool  mysql;
    bool  simon;
    int   verbosity_count;

    char* mysql_user;
    char* mysql_serv;
    char* mysql_dbnm;
    char* mysql_pass;

    Simulator sim;
    SimData* simdata;
    SimMap* simmap;
}
Parameters;

typedef enum
{
    A_PLAY          = 0,
    A_BROWSE        = 0
}
ProgramAction;

typedef enum
{
    E_SUCCESS_AND_EXIT = 0,
    E_SUCCESS_AND_DO   = 1,
    E_SOMETHING_BAD    = 2
}
ConfigError;

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
