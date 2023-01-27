#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <libconfig.h>

#include "gameloop/gameloop.h"
#include "helper/parameters.h"
#include "helper/dirhelper.h"
#include "helper/confighelper.h"
//#include "simulatorapi/simdata.h"
#include "slog/slog.h"



int create_dir(char* dir)
{
    struct stat st = {0};
    if (stat(dir, &st) == -1)
    {
        mkdir(dir, 0700);
    }
}

char* create_user_dir(char* dirtype)
{
    char* home_dir_str = gethome();
    char* config_dir_str = ( char* ) malloc(1 + strlen(home_dir_str) + strlen(dirtype) + strlen("gilles/"));
    strcpy(config_dir_str, home_dir_str);
    strcat(config_dir_str, dirtype);
    strcat(config_dir_str, "gilles/");

    create_dir(config_dir_str);
    free(config_dir_str);
}


int main(int argc, char** argv)
{

    Parameters* p = malloc(sizeof(Parameters));
    GillesSettings* gs = malloc(sizeof(GillesSettings));;

    ConfigError ppe = getParameters(argc, argv, p);
    if (ppe == E_SUCCESS_AND_EXIT)
    {
        goto cleanup_final;
    }
    gs->program_action = p->program_action;

    char* home_dir_str = gethome();
    create_user_dir("/.config/");
    create_user_dir("/.cache/");
    char* config_file_str = ( char* ) malloc(1 + strlen(home_dir_str) + strlen("/.config/") + strlen("gilles/gilles.config"));
    char* cache_dir_str = ( char* ) malloc(1 + strlen(home_dir_str) + strlen("/.cache/gilles/"));
    strcpy(config_file_str, home_dir_str);
    strcat(config_file_str, "/.config/");
    strcpy(cache_dir_str, home_dir_str);
    strcat(cache_dir_str, "/.cache/gilles/");
    strcat(config_file_str, "gilles/gilles.config");

    slog_config_t slgCfg;
    slog_config_get(&slgCfg);
    slgCfg.eColorFormat = SLOG_COLORING_TAG;
    slgCfg.eDateControl = SLOG_TIME_ONLY;
    strcpy(slgCfg.sFileName, "gilles.log");
    strcpy(slgCfg.sFilePath, cache_dir_str);
    slgCfg.nTraceTid = 0;
    slgCfg.nToScreen = 1;
    slgCfg.nUseHeap = 0;
    slgCfg.nToFile = 1;
    slgCfg.nFlush = 0;
    slgCfg.nFlags = SLOG_FLAGS_ALL;
    slog_config_set(&slgCfg);
    if (p->verbosity_count < 2)
    {
        slog_disable(SLOG_TRACE);
    }
    if (p->verbosity_count < 1)
    {
        slog_disable(SLOG_DEBUG);
    }
    /*
    slogi("Loading configuration file: %s", config_file_str);
    config_t cfg;
    config_init(&cfg);
    config_setting_t* config_devices = NULL;

    Metric* m = malloc(sizeof(Metric));
    if (!config_read_file(&cfg, config_file_str))
    {
        fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
    }
    else
    {
        slogi("Openend raceengineer configuration file");
        config_devices = config_lookup(&cfg, "monitor");
        config_setting_t* config_device = config_setting_get_elem(config_devices, 0);



        config_setting_lookup_string(config_device, "name", &m->name);

        config_setting_lookup_string(config_device, "afile0", &m->afile0);
        config_setting_lookup_string(config_device, "afile1", &m->afile1);
        config_setting_lookup_string(config_device, "afile2", &m->afile2);

        config_setting_lookup_float(config_device, "thresh1", &m->thresh1);
        config_setting_lookup_float(config_device, "thresh2", &m->thresh2);
    }
    */
    looper(1, p);

    free(config_file_str);
    free(cache_dir_str);



configcleanup:
    //config_destroy(&cfg);

cleanup_final:
    free(gs);
    free(p);
    exit(0);
}


