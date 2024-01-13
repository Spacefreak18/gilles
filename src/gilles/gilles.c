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
#include "gameloop/browseloop.h"
#include "helper/parameters.h"
#include "helper/dirhelper.h"
#include "helper/confighelper.h"
#include "slog/slog.h"

#define PROGRAM_NAME "gilles"

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

    char* cachedir = create_user_dir(home_dir_str, ".cache", PROGRAM_NAME);

    slog_config_t slgCfg;
    slog_config_get(&slgCfg);
    slgCfg.eColorFormat = SLOG_COLORING_TAG;
    slgCfg.eDateControl = SLOG_TIME_ONLY;
    strcpy(slgCfg.sFileName, "gilles.log");
    strcpy(slgCfg.sFilePath, cachedir);
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

    char* configdir = create_user_dir(home_dir_str, ".config", PROGRAM_NAME);
    char* datadir = create_user_dir(home_dir_str, ".local/share", PROGRAM_NAME);


    xdgHandle xdg;
    if(!xdgInitHandle(&xdg))
    {
        slogf("Function xdgInitHandle() failed, is $HOME unset?");
    }

    char* config_file_str = get_config_file(p->config_path, &xdg);
    int err = loadconfig(config_file_str, p);

    free(config_file_str);
    xdgWipeHandle(&xdg);

    if (err == E_NO_ERROR)
    {
        if (p->program_action == A_PLAY)
        {
            mainloop(p);
        }
        else
        {
            browseloop(p, datadir);
        }
        if (p->err != E_NO_ERROR)
        {
            sloge("Error occured during execution.");
        }
    }


configcleanup:
    //config_destroy(&cfg);

cleanup_final:
    freeparams(p);
    free(gs);
    free(p);

    free(configdir);
    free(datadir);
    free(cachedir);

    exit(0);
}

