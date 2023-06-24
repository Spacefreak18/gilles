#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "confighelper.h"

#include "../slog/slog.h"

//int strtogame(const char* game, GillesSettings* gs)
//{
//    slogd("Checking for %s in list of supported simulators.", game);
//    if (strcmp(game, "ac") == 0)
//    {
//        slogd("Setting simulator to Assetto Corsa");
//        gs->sim_name = SIMULATOR_ASSETTO_CORSA;
//    }
//    else
//        if (strcmp(game, "test") == 0)
//        {
//            slogd("Setting simulator to Test Data");
//            gs->sim_name = SIMULATOR_GILLES_TEST;
//        }
//        else
//        {
//            slogi("%s does not appear to be a supported simulator.", game);
//            return GILLES_ERROR_INVALID_SIM;
//        }
//    return GILLES_ERROR_NONE;
//}

int loadmysql(config_t* cfg, Parameters* p)
{
    config_setting_t* config_mysql_array = NULL;
    config_mysql_array = config_lookup(cfg, "mysql");

    config_setting_t* config_mysql = NULL;
    config_mysql = config_setting_get_elem(config_mysql_array, 0);

    if (config_mysql == NULL) {
        slogi("found no mysql settings");
    }
    const char* temp;
    config_setting_lookup_string(config_mysql, "user", &temp);
    p->mysql_user = strdup(temp);
    return 0;
}

int loadconfig(const char* config_file_str, Parameters* p)
{
    config_t cfg;
    config_init(&cfg);
    if (!config_read_file(&cfg, config_file_str))
    {
        fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
    }
    else
    {
        slogi("Parsing config file");
        if (p->mysql == true)
        {
            loadmysql(&cfg, p);
        }
    }

    config_destroy(&cfg);

    return 0;
}

