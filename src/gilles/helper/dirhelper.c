#include "dirhelper.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>

#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>


#include <string.h>



#include "../slog/slog.h"


#define CONFIG_FILE "gilles/gilles.config"

void create_dir(char* dir)
{
    struct stat st = {0};
    if (stat(dir, &st) == -1)
    {
        mkdir(dir, 0700);
    }
}

char* create_user_dir(char* home_dir_str, const char* dirtype, const char* programname)
{
    // +3 for slashes
    size_t ss = (4 + strlen(home_dir_str) + strlen(dirtype) + strlen(programname));
    char* config_dir_str = malloc(ss);

    snprintf (config_dir_str, ss, "%s/%s/%s/", home_dir_str, dirtype, programname);

    slogt("creating dir for %s if necessary", config_dir_str);
    create_dir(config_dir_str);
    return config_dir_str;
}


char* get_config_file(const char* confpath, xdgHandle* xdg)
{
    if ((confpath != NULL) && (strcmp(confpath, "") != 0))
    {
        slogw("Using custom config path %s", confpath);
        return strdup(confpath);
    }

    const char* relpath = CONFIG_FILE;
    char* confpath1 = xdgConfigFind(relpath, xdg);
    slogi("config path is %s", confpath1);
    return confpath1;
}


char* get_dir_with_default(const char* dirpath, char* defaultpath)
{
    if ((dirpath != NULL) && (strcmp(dirpath, "") != 0))
    {
        slogw("Using custom config path %s", dirpath);
        return strdup(dirpath);
    }

    return defaultpath;
}

char* gethome()
{
    char* homedir = getenv("HOME");
    return homedir;

    if (homedir != NULL)
    {
        printf("Home dir in enviroment");
        printf("%s\n", homedir);
    }

    uid_t uid = getuid();
    struct passwd* pw = getpwuid(uid);

    if (pw == NULL)
    {
        printf("Failed\n");
        exit(EXIT_FAILURE);
    }

    return pw->pw_dir;
}

time_t get_file_creation_time(char* path)
{
    struct stat attr;
    stat(path, &attr);
    return attr.st_mtime;
}

void delete_dir(char* path)
{

    struct dirent* de;
    DIR* dr = opendir(path);

    if (dr == NULL)
    {
        printf("Could not open current directory");
    }

    // Refer http://pubs.opengroup.org/onlinepubs/7990989775/xsh/readdir.html
    while ((de = readdir(dr)) != NULL)
    {
        char* fullpath = ( char* ) malloc(1 + strlen(path) + strlen("/") + strlen(de->d_name));
        strcpy(fullpath, path);
        strcat(fullpath, "/");
        strcat(fullpath, de->d_name);
        unlink(fullpath);
        free(fullpath);
    }
    closedir(dr);
    rmdir(path);

}

void delete_oldest_dir(char* path)
{
    char* oldestdir = path;

    struct dirent* de;
    DIR* dr = opendir(path);

    if (dr == NULL)
    {
        printf("Could not open current directory");
    }

    // Refer http://pubs.opengroup.org/onlinepubs/7990989775/xsh/readdir.html
    char filename_qfd[100] ;
    char* deletepath = NULL;
    time_t tempoldest = 0;
    while ((de = readdir(dr)) != NULL)
    {
        struct stat stbuf;

        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
        {
            continue;
        }

        char* fullpath = ( char* ) malloc(1 + strlen(path) + strlen(de->d_name));
        strcpy(fullpath, path);
        strcat(fullpath, de->d_name);

        stat(fullpath, &stbuf);
        if (S_ISDIR(stbuf.st_mode))
        {
            strcpy(fullpath, path);
            strcat(fullpath, de->d_name);
            if (tempoldest == 0)
            {
                tempoldest = get_file_creation_time(fullpath);
                free(deletepath);
                deletepath = strdup(fullpath);
            }
            else
            {
                time_t t = get_file_creation_time(fullpath);
                double diff = tempoldest - t;
                if (diff > 0)
                {
                    tempoldest = t;
                    free(deletepath);
                    deletepath = strdup(fullpath);
                }
            }

        }

        free(fullpath);
    }
    closedir(dr);
    delete_dir(deletepath);
    free(deletepath);
}

void restrict_folders_to_cache(char* path, int cachesize)
{
    int numfolders = 0;

    struct dirent* de;
    DIR* dr = opendir(path);

    if (dr == NULL)
    {
        printf("Could not open current directory");
    }

    // Refer http://pubs.opengroup.org/onlinepubs/7990989775/xsh/readdir.html
    while ((de = readdir(dr)) != NULL)
    {

        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
        {
            continue;
        }

        char* fullpath = ( char* ) malloc(1 + strlen(path) + strlen(de->d_name));
        strcpy(fullpath, path);
        strcat(fullpath, de->d_name);
        strcat(fullpath, "/");

        struct stat stbuf;
        stat(fullpath,&stbuf);

        if (S_ISDIR(stbuf.st_mode))
        {
            numfolders++;
        }
        free(fullpath);
    }

    while (numfolders >= cachesize)
    {
        delete_oldest_dir(path);
        numfolders--;
    }
    closedir(dr);

}

bool does_directory_exist(char* path, char* dirname)
{
    struct dirent* de;
    DIR* dr = opendir(path);

    if (dr == NULL)
    {
        printf("Could not open current directory");
        return false;
    }

    // Refer http://pubs.opengroup.org/onlinepubs/7990989775/xsh/readdir.html
    bool answer = false;
    while ((de = readdir(dr)) != NULL)
    {
        if (strcmp(dirname,de->d_name) == 0)
        {
            answer = true;
        }
    }

    closedir(dr);
    return answer;
}

