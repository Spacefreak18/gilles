#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <libconfig.h>
#include <pthread.h>
#include <basedir_fs.h>

#include "gameloop/gameloop.h"
#include "helper/parameters.h"
#include "helper/dirhelper.h"
#include "helper/confighelper.h"
#include "slog/slog.h"

int create_dir(char* dir)
{
    struct stat st = {0};
    if (stat(dir, &st) == -1)
    {
        mkdir(dir, 0700);
    }
}

char* create_user_dir(const char* dirtype)
{
    char* config_dir_str = ( char* ) malloc(1 + strlen(dirtype) + strlen("/gilles"));
    strcpy(config_dir_str, dirtype);
    strcat(config_dir_str, "/gilles");

    create_dir(config_dir_str);
    free(config_dir_str);
}

char* get_config_file(const char* confpath, xdgHandle* xdg)
{
    if(strcmp(confpath, "") != 0)
    {
        fprintf(stderr, "no config path specified");
        return strdup(confpath);
    }

    const char* relpath = "gilles/gilles.config";
    const char* confpath1 = xdgConfigFind(relpath, xdg);
    slogi("path is %s", confpath1);
    return strdup(confpath1);
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
    p->program_state = 1;

    char* home_dir_str = gethome();
    create_user_dir("/.config/");
    create_user_dir("/.cache/");
    char* cache_dir_str = ( char* ) malloc(1 + strlen(home_dir_str) + strlen("/.cache/gilles/"));
    strcpy(cache_dir_str, home_dir_str);
    strcat(cache_dir_str, "/.cache/gilles/");

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

    xdgHandle xdg;
    if(!xdgInitHandle(&xdg))
    {
        slogf("Function xdgInitHandle() failed, is $HOME unset?");
    }
    char* config_file_str = get_config_file("/home/paul/.config/gilles/gilles.config", &xdg);

    loadconfig(config_file_str, p);
    //slogi("mysql user is %s", p->mysql_user);

    free(config_file_str);
    free(cache_dir_str);
    xdgWipeHandle(&xdg);

    mainloop(p);
    //free(config_file_str);
    //free(cache_dir_str);
    //free(simmap);
    //free(simdata);


configcleanup:
    //config_destroy(&cfg);

cleanup_final:
    freeparams(p);
    free(gs);
    free(p);
    exit(0);
}


