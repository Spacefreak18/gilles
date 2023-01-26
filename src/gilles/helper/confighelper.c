#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>


#include "confighelper.h"

#include "../slog/slog.h"

int strtogame(const char* game, GillesSettings* gs)
{
    slogd("Checking for %s in list of supported simulators.", game);
    if (strcmp(game, "ac") == 0)
    {
        slogd("Setting simulator to Assetto Corsa");
        gs->sim_name = SIMULATOR_ASSETTO_CORSA;
    }
    else
        if (strcmp(game, "test") == 0)
        {
            slogd("Setting simulator to Test Data");
            gs->sim_name = SIMULATOR_GILLES_TEST;
        }
        else
        {
            slogi("%s does not appear to be a supported simulator.", game);
            return GILLES_ERROR_INVALID_SIM;
        }
    return GILLES_ERROR_NONE;
}


int loadconfig(const char* config_file)
{
    return 0;
}

