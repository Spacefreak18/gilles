#ifndef _CONFIGHELPER_H
#define _CONFIGHELPER_H

#include <stdbool.h>
#include <stdint.h>

#include <libconfig.h>

#include "parameters.h"

typedef enum
{
    SIMULATOR_UPDATE_DEFAULT    = 0,
    SIMULATOR_UPDATE_RPMS       = 1,
    SIMULATOR_UPDATE_GEAR       = 2,
    SIMULATOR_UPDATE_PULSES     = 3,
    SIMULATOR_UPDATE_VELOCITY   = 4,
}
SimulatorUpdate;

typedef struct
{
    ProgramAction program_action;
    Simulator sim_name;
}
GillesSettings;

int loadconfig(const char* config_file_str, Parameters* p);

int strtogame(const char* game, GillesSettings* gs);

#endif
