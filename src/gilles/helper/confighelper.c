#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "confighelper.h"

#include "../slog/slog.h"
#include "parameters.h"

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

int loaddbconfig(config_t* cfg, Parameters* p)
{
    config_setting_t* config_db_array = NULL;
    config_db_array = config_lookup(cfg, "db");

    config_setting_t* config_db = NULL;
    config_db = config_setting_get_elem(config_db_array, 0);

    if (config_db == NULL) {
        slogi("found no db settings");
        return E_BAD_CONFIG;
    }
    const char* temp;
    config_setting_lookup_string(config_db, "user", &temp);
    p->db_user = strdup(temp);
    config_setting_lookup_string(config_db, "password", &temp);
    p->db_pass = strdup(temp);
    config_setting_lookup_string(config_db, "server", &temp);
    p->db_serv = strdup(temp);
    config_setting_lookup_string(config_db, "database", &temp);
    p->db_dbnm = strdup(temp);

    size_t strsize = strlen(p->db_user) + strlen(p->db_pass) + strlen(p->db_serv) + strlen(p->db_dbnm) + 29 + 1;
    p->db_conn = malloc(strsize);

    snprintf(p->db_conn, strsize, "host=%s dbname=%s user=%s password=%s", p->db_serv, p->db_dbnm, p->db_user, p->db_pass);

    return E_NO_ERROR;
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


        if (p->program_action == A_BROWSE)
        {
            char* gnuplotbin = "/usr/bin/gnuplot";
            config_setting_t* config_graph_array = NULL;
            config_graph_array = config_lookup(&cfg, "graph");

            config_setting_t* config_graph = NULL;
            config_graph = config_setting_get_elem(config_graph_array, 0);

            if (config_graph == NULL) {
                slogi("found no graph settings");
                return E_BAD_CONFIG;
            }
            const char* temp;
            config_setting_lookup_string(config_graph, "gnuplotfile", &temp);
            p->gnuplot_file = strdup(temp);
            slogt("set gnuplot config file %s", p->gnuplot_file);

            int found = config_setting_lookup_string(config_graph, "gnuplotbin", &temp);
            if (found > 0)
            {
                p->gnuplot_bin = strdup(temp);
            }
            else
            {
                p->gnuplot_bin = strdup(gnuplotbin);
            }
            p->gnuplotfound = 0;
            if (access(p->gnuplot_bin, F_OK) == 0)
            {
                p->gnuplotfound = 1;
            }
        }

        if (p->mysql == true || p->program_action == A_BROWSE)
        {
            int err = loaddbconfig(&cfg, p);
            if (err != E_NO_ERROR)
            {
                return err;
            }
        }
    }

    config_destroy(&cfg);

    return 0;
}

