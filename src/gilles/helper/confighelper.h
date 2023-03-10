#ifndef _CONFIGHELPER_H
#define _CONFIGHELPER_H

#include <stdbool.h>
#include <stdint.h>

#include <libconfig.h>

#include "parameters.h"

typedef enum
{
    SIMULATOR_GILLES_TEST    = 0,
    SIMULATOR_ASSETTO_CORSA     = 1
}
Simulator;

typedef enum
{
    SIMULATOR_UPDATE_DEFAULT    = 0,
    SIMULATOR_UPDATE_RPMS       = 1,
    SIMULATOR_UPDATE_GEAR       = 2,
    SIMULATOR_UPDATE_PULSES     = 3,
    SIMULATOR_UPDATE_VELOCITY   = 4,
}
SimulatorUpdate;

typedef enum
{
    GILLES_ERROR_NONE          = 0,
    GILLES_ERROR_UNKNOWN       = 1,
    GILLES_ERROR_INVALID_SIM   = 2,
    GILLES_ERROR_NODATA        = 4,
}
GillesError;

typedef struct
{
    ProgramAction program_action;
    Simulator sim_name;
}
GillesSettings;

int strtogame(const char* game, GillesSettings* gs);

#endif
