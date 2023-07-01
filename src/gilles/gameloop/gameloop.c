#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <MQTTClient.h>
#include <poll.h>
#include <termios.h>
#include <hoel.h>
#include <jansson.h>

#include "gameloop.h"
#include "../helper/parameters.h"
#include "../helper/confighelper.h"
#include "../helper/dirhelper.h"
#include "../simulatorapi/simapi/simapi/simdata.h"
#include "../simulatorapi/simapi/simapi/simmapper.h"
#include "../slog/slog.h"

#define DEFAULT_UPDATE_RATE      60
#define DATA_UPDATE_RATE         32
#define TRACK_SAMPLE_RATE        4
#define SIM_CHECK_RATE           1

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "gilles"
#define TOPIC       "telemetry"
//#define PAYLOAD     "Hello, MQTT!"
#define QOS         0
#define TIMEOUT     10000L

char datestring[30];

WINDOW* win1;
WINDOW* win2;
WINDOW* win3;
WINDOW* win4;

int winx, winy;

int win23y, win23x;



void handle_winch(int sig)
{
    endwin();

    refresh();
    clear();
    getmaxyx(stdscr, winx, winy);
    win23y = winy/3;
    win23x = winx/3;
    win1 = newwin(winx,winy,0,0);
    win2 = newwin(win23x,win23y,1,win23y-1);
    win3 = newwin(win23x,win23y,1,win23y*2-1);
    win4 = newwin(winx-win23x-2,winy-win23y,win23x+1,win23y-1);
    refresh();
}

void rectangle(int y1, int x1, int y2, int x2)
{
    mvhline(y1, x1, 0, x2-x1);
    mvhline(y2, x1, 0, x2-x1);
    mvvline(y1, x1, 0, y2-y1);
    mvvline(y1, x2, 0, y2-y1);
    mvaddch(y1, x1, ACS_ULCORNER);
    mvaddch(y2, x1, ACS_LLCORNER);
    mvaddch(y1, x2, ACS_URCORNER);
    mvaddch(y2, x2, ACS_LRCORNER);
}

int curses_init()
{
    initscr();
    start_color();

    init_pair(1,COLOR_GREEN,0);
    init_pair(2,COLOR_YELLOW,0);
    init_pair(3,COLOR_MAGENTA,0);
    init_pair(4,COLOR_WHITE,0);

    getmaxyx(stdscr, winx, winy);
    win1 = newwin(winx,winy,0,0);
    win23y = winy/3;
    win23x = winx/3;
    win2 = newwin(win23x,win23y,1,win23y-1);
    win3 = newwin(win23x,win23y,1,win23y*2-1);
    win4 = newwin(winx-win23x-2,winy-win23y,win23x+1,win23y-1);

    wbkgd(win1,COLOR_PAIR(1));
    wbkgd(win2,COLOR_PAIR(1));
    wbkgd(win3,COLOR_PAIR(1));
    wbkgd(win4,COLOR_PAIR(1));

    signal(SIGWINCH, handle_winch);
    cbreak();
    noecho();

    box(win1, 0, 0);
    box(win2, 0, 0);
    box(win3, 0, 0);
    box(win4, 0, 0);
}

char * removeSpacesFromStr(char *string)
{
    int non_space_count = 0;

    for (int i = 0; string[i] != '\0'; i++)
    {
        if (string[i] != ' ')
        {
            string[non_space_count] = string[i];
            non_space_count++;
        }
    }

    string[non_space_count] = '\0';
    return string;
}

void update_date()
{
    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    sprintf(datestring, "%.24s", asctime (timeinfo));
}


int mainloop(Parameters* p)
{
    pthread_t ui_thread;
    pthread_t mqtt_thread;
    pthread_t mysql_thread;

    SimData* simdata = malloc(sizeof(SimData));
    SimMap* simmap = malloc(sizeof(SimMap));

    struct termios newsettings, canonicalmode;
    tcgetattr(0, &canonicalmode);
    newsettings = canonicalmode;
    newsettings.c_lflag &= (~ICANON & ~ECHO);
    newsettings.c_cc[VMIN] = 1;
    newsettings.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &newsettings);
    char ch;
    struct pollfd mypoll = { STDIN_FILENO, POLLIN|POLLPRI };

    p->simdata = simdata;
    p->simmap = simmap;
    fprintf(stdout, "Searching for sim data... Press q to quit...\n");

    double update_rate = SIM_CHECK_RATE;
    int go = true;
    char lastsimstatus = false;
    while (go == true)
    {





        // check for running sims
        if (file_exists("/dev/shm/acpmf_physics"))
        {
            if (file_exists("/dev/shm/acpmf_static"))
            {
                p->sim = SIMULATOR_ASSETTO_CORSA;
                int error = siminit(simdata, simmap, 1);
                simdatamap(simdata, simmap, 1);
                if (error == 0 && simdata->simstatus > 1)
                {
                    slogi("found Assetto Corsa, starting application...");
                    p->simon = true;
                }
            }
        }

        if (p->simon == true)
        {
            if (p->cli == true)
            {
                if (pthread_create(&ui_thread, NULL, &clilooper, p) != 0)
                {
                    printf("Uh-oh!\n");
                    return -1;
                }
            }
            else
            {
                if (pthread_create(&ui_thread, NULL, &looper, p) != 0)
                {
                    printf("Uh-oh!\n");
                    return -1;
                }
            }
            //if (p->mqtt == true)
            //{
            //    if (pthread_create(&mqtt_thread, NULL, &b4madmqtt, p) != 0)
            //    {
            //        printf("Uh-oh!\n");
            //        return -1;
            //    }
            //}
            if (p->mysql == true)
            {
                if (pthread_create(&mysql_thread, NULL, &simviewmysql, p) != 0)
                {
                    printf("Uh-oh!\n");
                    return -1;
                }
            }

            pthread_join(ui_thread, NULL);
            p->program_state = -1;
            //if (p->mqtt == true)
            //{
            //    pthread_join(mqtt_thread, NULL);
            //}
            if (p->mysql == true)
            {
                pthread_join(mysql_thread, NULL);
            }
        }

        if (p->simon == true)
        {
            p->simon = false;
            fprintf(stdout, "Searching for sim data... Press q again to quit...\n");
            sleep(2);
        }

        if( poll(&mypoll, 1, 1000.0/update_rate) )
        {
            scanf("%c", &ch);
            if(ch == 'q')
            {
                go = false;
            }
        }
    }

    fprintf(stdout, "\n");
    fflush(stdout);
    tcsetattr(0, TCSANOW, &canonicalmode);

    free(simdata);
    free(simmap);

    return 0;
}

void* clilooper(void* thargs)
{
    Parameters* p = (Parameters*) thargs;
    SimData* simdata = p->simdata;
    SimMap* simmap = p->simmap;

    struct termios newsettings, canonicalmode;
    tcgetattr(0, &canonicalmode);
    newsettings = canonicalmode;
    newsettings.c_lflag &= (~ICANON & ~ECHO);
    newsettings.c_cc[VMIN] = 1;
    newsettings.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &newsettings);
    char ch;
    struct pollfd mypoll = { STDIN_FILENO, POLLIN|POLLPRI };

    fprintf(stdout, "Press q to quit...\n");
    fprintf(stdout, "Press c for a useful readout of car telemetry...\n");
    fprintf(stdout, "Press s for basic sesion information...\n");
    fprintf(stdout, "Press l for basic lap / stint information...\n");

    double update_rate = DEFAULT_UPDATE_RATE;
    int go = true;
    char lastsimstatus = false;
    while (go == true && simdata->simstatus > 1)
    {
        simdatamap(simdata, simmap, p->sim);

        if( poll(&mypoll, 1, 1000.0/update_rate) )
        {
            scanf("%c", &ch);
            if(ch == 'q')
            {
                go = false;
            }
            if(ch == 'c')
            {
                slogi("speed: %i gear: %i", simdata->velocity, simdata->gear);
            }
            if(ch == 's')
            {
                slogi("status: %i", simdata->simstatus);
            }
        }
    }

    fprintf(stdout, "\n");
    fflush(stdout);
    tcsetattr(0, TCSANOW, &canonicalmode);

    return 0;
}

void* looper(void* thargs)
{
    Parameters* p = (Parameters*) thargs;
    SimData* simdata = p->simdata;
    SimMap* simmap = p->simmap;

    curses_init();

    timeout(DEFAULT_UPDATE_RATE);

    int go = true;
    char lastsimstatus = false;
    while (go == true)
    {
        simdatamap(simdata, simmap, p->sim);

        wclear(win1);
        wclear(win2);
        wclear(win3);
        wclear(win4);

        { // window 1 car diagnostics

            char spacer[14];
            sprintf(spacer, "\n");
            waddstr(win1, spacer);

            sprintf(spacer, "\n");
            waddstr(win1, spacer);

            char speed[14];
            wbkgd(win1,COLOR_PAIR(1));
            wattrset(win1, COLOR_PAIR(1));
            wprintw(win1, "   Speed: ");
            sprintf(speed, "%i\n", simdata->velocity);
            wattrset(win1, COLOR_PAIR(2));
            waddstr(win1, speed);

            char rpm[14];
            wattrset(win1, COLOR_PAIR(1));
            wprintw(win1, "   RPM: ");
            sprintf(rpm, "%i\n", simdata->rpms);
            wattrset(win1, COLOR_PAIR(2));
            waddstr(win1, rpm);

            char gear[14];
            wattrset(win1, COLOR_PAIR(1));
            wprintw(win1, "   Gear: ");
            sprintf(gear, "%i\n", simdata->gear);
            wattrset(win1, COLOR_PAIR(2));
            waddstr(win1, gear);

            char gas[14];
            wattrset(win1, COLOR_PAIR(1));
            wprintw(win1, "   Gas: ");
            sprintf(gas, "%f\n", simdata->gas);
            wattrset(win1, COLOR_PAIR(2));
            waddstr(win1, gas);

            char brake[14];
            wattrset(win1, COLOR_PAIR(1));
            wprintw(win1, "   Brake: ");
            sprintf(brake, "%f\n", simdata->brake);
            wattrset(win1, COLOR_PAIR(2));
            waddstr(win1, brake);

            char fuel[14];
            wattrset(win1, COLOR_PAIR(1));
            wprintw(win1, "   Fuel: ");
            sprintf(fuel, "%f\n", simdata->fuel);
            wattrset(win1, COLOR_PAIR(2));
            waddstr(win1, fuel);

            sprintf(spacer, "\n");
            waddstr(win1, spacer);

            sprintf(spacer, "\n");
            waddstr(win1, spacer);

            sprintf(spacer, "\n");
            waddstr(win1, spacer);

            wattrset(win1, COLOR_PAIR(1));
            wprintw(win1, "  braketemp ");
            wattrset(win1, COLOR_PAIR(2));
            wprintw(win1, "%.0f", simdata->braketemp[0]);
            wattrset(win1, COLOR_PAIR(1));
            wprintw(win1, "      braketemp ");
            wattrset(win1, COLOR_PAIR(2));
            wprintw(win1, "%.0f\n", simdata->braketemp[1]);

            sprintf(spacer, "\n");
            waddstr(win1, spacer);

            wattrset(win1, COLOR_PAIR(1));
            wprintw(win1, "  temp ");
            wattrset(win1, COLOR_PAIR(2));
            wprintw(win1, "%.0f", simdata->tyretemp[0]);
            wattrset(win1, COLOR_PAIR(1));
            wprintw(win1, "               temp ");
            wattrset(win1, COLOR_PAIR(2));
            wprintw(win1, "%.0f\n", simdata->tyretemp[1]);

            wattrset(win1, COLOR_PAIR(1));
            wprintw(win1, "  pres ");
            wattrset(win1, COLOR_PAIR(2));
            wprintw(win1, "%.0f", simdata->tyrepressure[0]);
            wattrset(win1, COLOR_PAIR(1));
            wprintw(win1, "               pres ");
            wattrset(win1, COLOR_PAIR(2));
            wprintw(win1, "%.0f\n", simdata->tyrepressure[1]);

            wattrset(win1, COLOR_PAIR(1));
            wprintw(win1, "  wear ");
            wattrset(win1, COLOR_PAIR(2));
            wprintw(win1, "%.0f", simdata->tyrewear[0]);
            wattrset(win1, COLOR_PAIR(1));
            wprintw(win1, "               wear ");
            wattrset(win1, COLOR_PAIR(2));
            wprintw(win1, "%.0f\n", simdata->tyrewear[1]);

            sprintf(spacer, "\n");
            waddstr(win1, spacer);

            sprintf(spacer, "\n");
            waddstr(win1, spacer);

            sprintf(spacer, "\n");
            waddstr(win1, spacer);

            wattrset(win1, COLOR_PAIR(1));
            wprintw(win1, "  temp ");
            wattrset(win1, COLOR_PAIR(2));
            wprintw(win1, "%.0f", simdata->tyretemp[2]);
            wattrset(win1, COLOR_PAIR(1));
            wprintw(win1, "               temp ");
            wattrset(win1, COLOR_PAIR(2));
            wprintw(win1, "%.0f\n", simdata->tyretemp[3]);

            wattrset(win1, COLOR_PAIR(1));
            wprintw(win1, "  pres ");
            wattrset(win1, COLOR_PAIR(2));
            wprintw(win1, "%.0f", simdata->tyrepressure[2]);
            wattrset(win1, COLOR_PAIR(1));
            wprintw(win1, "               pres ");
            wattrset(win1, COLOR_PAIR(2));
            wprintw(win1, "%.0f\n", simdata->tyrepressure[3]);

            wattrset(win1, COLOR_PAIR(1));
            wprintw(win1, "  wear ");
            wattrset(win1, COLOR_PAIR(2));
            wprintw(win1, "%.0f", simdata->tyrewear[2]);
            wattrset(win1, COLOR_PAIR(1));
            wprintw(win1, "               wear ");
            wattrset(win1, COLOR_PAIR(2));
            wprintw(win1, "%.0f\n", simdata->tyrewear[3]);

            sprintf(spacer, "\n");
            waddstr(win1, spacer);

            wattrset(win1, COLOR_PAIR(1));
            wprintw(win1, "  braketemp ");
            wattrset(win1, COLOR_PAIR(2));
            wprintw(win1, "%.0f", simdata->braketemp[0]);
            wattrset(win1, COLOR_PAIR(1));
            wprintw(win1, "      braketemp ");
            wattrset(win1, COLOR_PAIR(2));
            wprintw(win1, "%.0f\n", simdata->braketemp[1]);

            wattrset(win1, COLOR_PAIR(1));
            rectangle(12, 12, 16, 16);  // left front
            rectangle(12, 18, 16, 22); // right front
            rectangle(18, 12, 22, 16);  // left rear
            rectangle(18, 18, 22, 22); // right rear
        }

        { // window 2 session info

            char spacer[14];
            sprintf(spacer, "\n");
            waddstr(win2, spacer);

            char airtemp[14];
            wbkgd(win2,COLOR_PAIR(1));
            wattrset(win2, COLOR_PAIR(1));
            wprintw(win2, "   Air Temp: ");
            sprintf(airtemp, "%.0f\n", simdata->airtemp);
            wattrset(win2, COLOR_PAIR(2));
            waddstr(win2, airtemp);

            char airdensity[14];
            wattrset(win2, COLOR_PAIR(1));
            wprintw(win2, "   Humidity: ");
            sprintf(airdensity, "%.0f\n", simdata->airdensity);
            wattrset(win2, COLOR_PAIR(2));
            waddstr(win2, airdensity);

            char tracktemp[14];
            wattrset(win2, COLOR_PAIR(1));
            wprintw(win2, "   Track Temp: ");
            sprintf(tracktemp, "%.0f\n", simdata->tracktemp);
            wattrset(win2, COLOR_PAIR(2));
            waddstr(win2, tracktemp);

            char numlaps[14];
            wbkgd(win2,COLOR_PAIR(1));
            wattrset(win2, COLOR_PAIR(1));
            wprintw(win2, "   Num Laps: ");
            sprintf(numlaps, "%i\n", simdata->numlaps);
            wattrset(win2, COLOR_PAIR(2));
            waddstr(win2, numlaps);

            char timeleft[14];
            int hours = simdata->timeleft/6000;
            int minutes = simdata->timeleft/60 - (hours*6000);
            int seconds = simdata->timeleft-((minutes*60)+(hours*6000));
            //int fraction = simdata->timeleft-(minutes*60000)-(seconds*1000);
            wbkgd(win2, COLOR_PAIR(1));
            wattrset(win2, COLOR_PAIR(1));
            wprintw(win2, "   Time Left: ");
            sprintf(timeleft, "%02d:%02d:%02d\n", hours, minutes, seconds);
            wattrset(win2, COLOR_PAIR(2));
            waddstr(win2, timeleft);

            char currenttime[14];
            hours = simdata->time/6000;
            minutes = simdata->time/60 - (hours*6000);
            seconds = simdata->time-((minutes*60)+(hours*6000));
            //fraction = simdata->time-(minutes*60000)-(seconds*1000);
            wbkgd(win2, COLOR_PAIR(1));
            wattrset(win2, COLOR_PAIR(1));
            wprintw(win2, "   Current Time: ");
            sprintf(currenttime, "%02d:%02d:%02d\n", hours, minutes, seconds);
            wattrset(win2, COLOR_PAIR(2));
            waddstr(win2, currenttime);

            wattrset(win2, COLOR_PAIR(1));
        }

        { // window 3 basic timing and scoring

            char spacer[14];
            sprintf(spacer, "\n");
            waddstr(win3, spacer);

            char car[14];
            wattrset(win3, COLOR_PAIR(1));
            wprintw(win3, "   Car: ");
            sprintf(car, "%s\n", simdata->car);
            wattrset(win3, COLOR_PAIR(2));
            waddstr(win3, car);

            char track[14];
            wattrset(win3, COLOR_PAIR(1));
            wprintw(win3, "   Track: ");
            sprintf(track, "%s\n", simdata->track);
            wattrset(win3, COLOR_PAIR(2));
            waddstr(win3, track);

            char driver[14];
            wattrset(win3, COLOR_PAIR(1));
            wprintw(win3, "   Driver: ");
            sprintf(driver, "%s\n", simdata->driver);
            wattrset(win3, COLOR_PAIR(2));
            waddstr(win3, driver);

            char lap[14];
            wbkgd(win3,COLOR_PAIR(1));
            wattrset(win3, COLOR_PAIR(1));
            wprintw(win3, "   Lap: ");
            sprintf(lap, "%i\n", simdata->lap);
            wattrset(win3, COLOR_PAIR(2));
            waddstr(win3, lap);

            char position[14];
            wattrset(win3, COLOR_PAIR(1));
            wprintw(win3, "   Position: ");
            sprintf(position, "%i\n", simdata->position);
            wattrset(win3, COLOR_PAIR(2));
            waddstr(win3, position);

            char lastlap[14];
            int minutes = simdata->lastlap/60000;
            int seconds = simdata->lastlap/1000-(minutes*60);
            int fraction = simdata->lastlap-(minutes*60000)-(seconds*1000);
            wattrset(win3, COLOR_PAIR(1));
            wprintw(win3, "   Last Lap: ");
            sprintf(lastlap, "%d:%02d:%02d\n", minutes, seconds, fraction);
            wattrset(win3, COLOR_PAIR(2));
            waddstr(win3, lastlap);

            char bestlap[14];
            minutes = simdata->bestlap/60000;
            seconds = simdata->bestlap/1000-(minutes*60);
            fraction = simdata->bestlap-(minutes*60000)-(seconds*1000);
            wattrset(win3, COLOR_PAIR(1));
            wprintw(win3, "   Best Lap: ");
            sprintf(bestlap, "%d:%02d:%02d\n", minutes, seconds, fraction);
            wattrset(win3, COLOR_PAIR(2));
            waddstr(win3, bestlap);

            wattrset(win3, COLOR_PAIR(1));
        }

        { // window 4 live standings timing and scoring
            char spacer[14];
            sprintf(spacer, "\n");
            waddstr(win4, spacer);

            wattrset(win4, COLOR_PAIR(4));
            wprintw(win4, "  P  ");
            wprintw(win4, "       Driver       ");
            wprintw(win4, "         Car        ");
            wprintw(win4, "     Laps ");
            wprintw(win4, "  Last    ");
            wprintw(win4, "   Best   ");
            wprintw(win4, "    Pit   ");
            wprintw(win4, "\n");

            // figuring out how many and which entrants to display while respecting term size
            // this will take some work and testing
            int thisx, thisy;
            getmaxyx(win4, thisx, thisy);
            int maxdispcars = thisy/5;
            int displaycars = maxdispcars;
            if (displaycars > MAXCARS)
            {
                displaycars = MAXCARS;
            }
            if (simdata->numcars < displaycars)
            {
                displaycars = simdata->numcars;
            }
            for(int i=0; i<displaycars; i++)
            {
                int ihold = i;
                for(i=0; i<displaycars; i++)
                {
                    if((ihold+1)==simdata->cars[i].pos)
                        break;
                }

                wattrset(win4, COLOR_PAIR(2));
                int maxstrlen = 20;
                wprintw(win4, " %02d ", simdata->cars[i].pos);
                wprintw(win4, " %-*.*s   ", maxstrlen, maxstrlen, simdata->cars[i].driver);
                wprintw(win4, " %-*.*s  ", maxstrlen, maxstrlen, simdata->cars[i].car);
                wprintw(win4, " %d  ", simdata->cars[i].lap);

                char clastlap[14];
                int lastlap = simdata->cars[i].lastlap;
                int minutes = lastlap/60000;
                int seconds = lastlap/1000-(minutes*60);
                int fraction = lastlap-(minutes*60000)-(seconds*1000);
                sprintf(clastlap, "%02d:%02d:%03d", minutes, seconds, fraction);
                waddstr(win4, clastlap);

                wprintw(win4, "   ");

                char cbestlap[14];
                int bestlap = simdata->cars[i].bestlap;
                minutes = bestlap/60000;
                seconds = bestlap/1000-(minutes*60);
                fraction = bestlap-(minutes*60000)-(seconds*1000);
                sprintf(cbestlap, "%02d:%02d:%03d", minutes, seconds, fraction);
                waddstr(win4, cbestlap);

                if(simdata->cars[i].inpitlane == 0 && simdata->cars[i].inpit == 0)
                {
                wattrset(win4, COLOR_PAIR(1));
                wprintw(win4, "  ontrack  ");
                }
                else
                {
                    if(simdata->cars[i].inpit > 0)
                    {
                        wprintw(win4, "     pit  ");
                    }
                    else
                    {
                        wprintw(win4, "  pitlane  ");
                    }
                }
                i = ihold;
                wprintw(win4, "\n");
            }
            wattrset(win4, COLOR_PAIR(1));
        }

        box(win1, 0, 0);
        box(win2, 0, 0);
        box(win3, 0, 0);
        box(win4, 0, 0);

        wrefresh(win1);
        wrefresh(win2);
        wrefresh(win3);
        wrefresh(win4);

        if (getch() == 'q')
        {
            go = false;
        }
    }

    wrefresh(win4);
    delwin(win4);
    endwin();

    wrefresh(win3);
    delwin(win3);
    endwin();

    wrefresh(win2);
    delwin(win2);
    endwin();

    wrefresh(win1);
    delwin(win1);
    endwin();


    //return 0;
}

int getLastInsertID(struct _h_connection* conn)
{
    json_t* last_id = (h_last_insert_id(conn));
    int id = json_integer_value(last_id);
    json_decref(last_id);
    return id;
}

int adddriver(struct _h_connection* conn, int driverid, const char* drivername)
{
    if (driverid > 0)
    {
        return driverid;
    }
    json_t *root = json_object();
    json_t *json_arr = json_array();

    json_object_set_new( root, "table", json_string("drivers") );
    json_object_set_new( root, "values", json_arr );

    json_t* values = json_object();
    json_object_set_new(values, "driver_name", json_string(drivername));
    json_object_set_new(values, "prev_name", json_string(drivername));
    json_object_set_new(values, "steam64_id", json_integer(17));
    json_object_set_new(values, "country", json_string("USA"));
    json_array_append(json_arr, values);
    int res = h_insert(conn, root, NULL);
    json_decref(root);
    json_decref(values);

    driverid = getLastInsertID(conn);
    return driverid;
}

int addtrackconfig(struct _h_connection* conn, int trackconfigid, const char* track, int length)
{
    //track_config_id | track_name | config_name | display_name | country | city | length
    if (trackconfigid > 0)
    {
        return trackconfigid;
    }
    json_t *root = json_object();
    json_t *json_arr = json_array();

    json_object_set_new( root, "table", json_string(track) );
    json_object_set_new( root, "values", json_arr );

    json_t* values = json_object();
    json_object_set_new(values, "track_name", json_string(track));
    json_object_set_new(values, "config_name", json_string("default"));
    json_object_set_new(values, "display_name", json_string(track));
    json_object_set_new(values, "country", json_string("USA"));
    json_object_set_new(values, "city", json_string("USA"));
    json_object_set_new(values, "length", json_integer(length));

    json_array_append(json_arr, values);
    int res = h_insert(conn, root, NULL);
    trackconfigid = getLastInsertID(conn);
    json_decref(root);
    json_decref(values);

    return trackconfigid;
}

int addevent(struct _h_connection* conn, int track_config)
{

    json_t *root = json_object();
    json_t *json_arr = json_array();

    json_object_set_new( root, "table", json_string("events") );
    json_object_set_new( root, "values", json_arr );

    json_t* values = json_object();
    json_object_set_new(values, "track_config_id", json_integer(track_config));
    json_object_set_new(values, "event_name", json_string("default"));
    //event_id | server_name | track_config_id | event_name | team_event | active | livery_preview | use_number | practice_duration | quali_duration | race_duration | race_duration_type | race_wait_time | race_extra_laps | reverse_grid_positions
    json_array_append(json_arr, values);
    int res = h_insert(conn, root, NULL);
    json_decref(root);
    json_decref(values);
    return getLastInsertID(conn);
}

int addsession(struct _h_connection* conn, int eventid, int eventtype, int airtemp, int tracktemp)
{

// session_id | event_id | event_type | track_time | session_name
// | start_time | duration_min | elapsed_ms | laps | weather |
// air_temp | road_temp | start_grip | current_grip | is_finished
// | finish_time | last_activity | http_port
    json_t *root = json_object();
    json_t *json_arr = json_array();

    json_object_set_new( root, "table", json_string("sessions") );
    json_object_set_new( root, "values", json_arr );

    json_t* values = json_object();
    json_object_set_new(values, "event_id", json_integer(eventid));
    json_object_set_new(values, "event_type", json_integer(1));
    json_object_set_new(values, "duration_min", json_integer(60));
    json_object_set_new(values, "session_name", json_string("default"));
    json_object_set_new(values, "last_activity", json_string("2023-06-27"));
    json_object_set_new(values, "air_temp", json_integer(airtemp));
    json_object_set_new(values, "road_temp", json_integer(tracktemp));
    json_object_set_new(values, "weather", json_string("Windy"));
    json_array_append(json_arr, values);
    int res = h_insert(conn, root, NULL);
    slogt("session insert response: %i", res);
    json_decref(root);
    json_decref(values);
    return getLastInsertID(conn);
}

int addstint(struct _h_connection* conn, int sessionid, int driverid, int carid)
{

// stints
// session_stint_id | driver_id | team_member_id | session_id | car_id | game_car_id | laps | valid_laps | best_lap_id | is_finished | started_at | finished_at
    json_t *root = json_object();
    json_t *json_arr = json_array();

    json_object_set_new( root, "table", json_string("stints") );
    json_object_set_new( root, "values", json_arr );

    json_t* values = json_object();

    json_object_set_new(values, "driver_id", json_integer(driverid));
    json_object_set_new(values, "session_id", json_integer(sessionid));
    json_object_set_new(values, "car_id", json_integer(carid));
    json_object_set_new(values, "game_car_id", json_integer(carid));
    json_object_set_new(values, "started_at", json_string("2023-06-25") );
    json_array_append(json_arr, values);
    int res = h_insert(conn, root, NULL);
    slogt("stint insert response: %i", res);
    json_decref(root);
    json_decref(values);
    return getLastInsertID(conn);
}

int addstintlap(struct _h_connection* conn, int stintid)
{

// stint laps
// stint_lap_id | stint_id | sector_1 | sector_2 | sector_3 | grip | tyre | time | cuts | crashes | car_crashes | max_speed | avg_speed | finished_at
    json_t *root = json_object();
    json_t *json_arr = json_array();

    json_object_set_new( root, "table", json_string("laps") );
    json_object_set_new( root, "values", json_arr );

    json_t* values = json_object();

    json_object_set_new(values, "stint_id", json_integer(stintid));
    json_object_set_new(values, "tyre", json_string("Vintage") );
    json_array_append(json_arr, values);
    int res = h_insert(conn, root, NULL);
    slogt("stint lap insert response: %i", res);
    json_decref(root);
    json_decref(values);
    return getLastInsertID(conn);
}

int addcar(struct _h_connection* conn, int carid, const char* carname)
{

    // car_id | display_name | car_name | manufacturer | car_class

    if (carid > 0)
    {
        return carid;
    }
    json_t *root = json_object();
    json_t *json_arr = json_array();

    json_object_set_new( root, "table", json_string("cars") );
    json_object_set_new( root, "values", json_arr );

    json_t* values = json_object();
    json_object_set_new(values, "display_name", json_string(carname));
    json_object_set_new(values, "car_name", json_string(carname));
    json_object_set_new(values, "manufacturer", json_string("Unknown"));
    json_object_set_new(values, "car_class", json_string("Unknown"));
    json_array_append(json_arr, values);
    int res = h_insert(conn, root, NULL);
    json_decref(root);
    json_decref(values);

    carid = getLastInsertID(conn);
    return carid;
}

int addtelemetry(struct _h_connection* conn, int points, int stintid)
{

    json_t *root = json_object();
    json_t *json_arr = json_array();

    json_object_set_new( root, "table", json_string("telemetry") );
    json_object_set_new( root, "values", json_arr );

    json_t* values = json_object();
    json_object_set_new(values, "lap_id", json_integer(stintid));
    json_object_set_new(values, "points", json_integer(points));
    json_array_append(json_arr, values);
    int res = h_insert(conn, root, NULL);
    json_decref(root);
    json_decref(values);

    int telemid = getLastInsertID(conn);
    return telemid;
}

int updatetelemetry(struct _h_connection* conn, int telemid, int size, const char* column, void* data)
{

    //char *pp = malloc((size*2)+1);
    char output[(size * 2) + 1];
    char *ppp = &output[0];
    unsigned char *p = data;
    int i;
    for (i=0; i<size; i++) {
        ppp += sprintf(ppp, "%02hhX", p[i]);
        //snprintf(pp, (size*2)+1, "%s%02hhX", pp, p[i]);
    }

    //char output2[((size*2)+3)];
    ////snprintf(output2, (size*2)+18, "%s%s%s", "decode('", output, "', 'hex')");
    //snprintf(output2, (size*2)+18, "%s %s%s", "decode('", output, "', 'hex')");

    //slogt("heres a string %s", output2);

    //json_t* j_query = json_pack("{sss[{siso}]}",
    //                   "table",
    //                   "lap_telemetry",
    //                   "values",
    //                     "lap_id",
    //                     lapid,
    //                     "steer",
    //                     json_pack("s", "decode('deadbeef', 'hex')"));
    char* query = malloc((sizeof(char)*71)+(sizeof(column))+(size*2)+1);
    sprintf(query, "UPDATE telemetry SET %s = decode('%s', 'hex') WHERE telemetry_id = %i", column, &output, telemid);
    int res1 = h_query_update(conn, query);
    //int res1 = h_insert(conn, j_query, NULL);
    slogt("got res %i", res1);
    free(query);
    //json_t *root = json_object();
    //json_t *json_arr = json_array();

    //json_object_set_new( root, "table", json_string("lap_telemetry") );
    //json_object_set_new( root, "values", json_arr );

    //json_t* values = json_object();
    //json_object_set_new(values, "lap_id", json_integer(lapid));
    //json_object_set_new(values, "steer", json_string(output2));
    //json_array_append(json_arr, values);
    //int res = h_insert(conn, root, NULL);
    //json_decref(root);
    //json_decref(values);

    return res1;
}


int gettrack(struct _h_connection* conn, const char* trackname)
{

    json_t *j_result;
    char* where_clause = h_build_where_clause(conn, "config_name=%s AND track_name=%s", "default", trackname);
    json_t* j_query = json_pack("{sss[s]s{s{ssss}}}", "table", "track_config", "columns", "track_config_id", "where", " ", "operator", "raw",
            "value", where_clause);

    //char* qq;
    int res = h_select(conn, j_query, &j_result, NULL);
    //slogi("here your query: %s", qq);
  // Deallocate j_query since it won't be needed anymore
  json_decref(j_query);
  h_free(where_clause);
  int track_config = -1;
  // Test query execution result
  if (res == H_OK) {
    // Print result
    char* dump = json_dumps(j_result, JSON_INDENT(2));
    slogi("json select result is\n%s", dump);
    int index1 = json_array_size(j_result);
    if (index1 == 0)
    {
        slogw("no config for this track");
    }
    else {
    json_t* jj = json_array_get(j_result, index1-1);
    track_config = json_integer_value(json_object_get(jj, "track_config_id"));
    }
    // Deallocate data result
    json_decref(j_result);
    free(dump);
  } else {
    sloge("Error executing select query: %d", res);
  }
  return track_config;
}

int getdriver(struct _h_connection* conn, const char* driver_name)
{
    json_t *j_result;
    char* where_clause = h_build_where_clause(conn, "driver_name=%s", driver_name);
    json_t* j_query = json_pack("{sss[s]s{s{ssss}}}","table", "drivers", "columns", "driver_id", "where", " ", "operator", "raw",
            "value", where_clause);

    slogi("Looking for driver named %s", driver_name);
    //char* qq;
    int res = h_select(conn, j_query, &j_result, NULL);
    //slogi("here your query: %s", qq);
  // Deallocate j_query since it won't be needed anymore
  json_decref(j_query);
  h_free(where_clause);
  int driver_id = -1;
  // Test query execution result
  if (res == H_OK) {
    // Print result
    char* dump = json_dumps(j_result, JSON_INDENT(2));
    slogi("json select result is\n%s", dump);
    int index1 = json_array_size(j_result);
    if (index1 == 0)
    {
        slogw("no driver by this name");
    }
    else {
    json_t* jj = json_array_get(j_result, index1-1);
    driver_id = json_integer_value(json_object_get(jj, "driver_id"));
    }
    // Deallocate data result
    json_decref(j_result);
    free(dump);
  } else {
    sloge("Error executing select query: %d", res);
  }
  return driver_id;
}

int getcar(struct _h_connection* conn, const char* carname)
{
    json_t *j_result;
    char* where_clause = h_build_where_clause(conn, "car_name=%s", carname);
    json_t* j_query = json_pack("{sss[s]s{s{ssss}}}","table", "cars", "columns", "car_id", "where", " ", "operator", "raw",
            "value", where_clause);

    slogi("Looking for car named %s", carname);
    //char* qq;
    int res = h_select(conn, j_query, &j_result, NULL);
    //slogi("here your query: %s", qq);
  // Deallocate j_query since it won't be needed anymore
  json_decref(j_query);
  h_free(where_clause);
  int car_id = -1;
  // Test query execution result
  if (res == H_OK) {
    // Print result
    char* dump = json_dumps(j_result, JSON_INDENT(2));
    slogi("json select result is\n%s", dump);
    int index1 = json_array_size(j_result);
    if (index1 == 0)
    {
        slogw("no car by this name");
    }
    else {
    json_t* jj = json_array_get(j_result, index1-1);
    car_id = json_integer_value(json_object_get(jj, "car_id"));
    }
    // Deallocate data result
    json_decref(j_result);
    free(dump);
  } else {
    sloge("Error executing select query: %d", res);
  }
  return car_id;
}

void print_bytes(void *ptr, int size)
{
    char *pp = malloc((size*2)+1);
    char output[(size * 2) + 1];
    char *ppp = &output[0];
    unsigned char *p = ptr;
    int i;
    for (i=0; i<size; i++) {
        slogt("%02hhX", p[i]);
        ppp += sprintf(ppp, "%02X", p[i]);
        //snprintf(pp, (size*2)+1, "%s%02hhX", pp, p[i]);
    }
    slogt("\n");
    slogt("bytes %s", output);
}



void* simviewmysql(void* thargs)
{
    Parameters* p = (Parameters*) thargs;
    SimData* simdata = p->simdata;
    SimMap* simmap = p->simmap;

    struct _h_result result;
    struct _h_connection * conn;
    char* connectionstring = "host=zorak.brak dbname=gilles user=test password=thisisatest";
    conn = h_connect_pgsql(connectionstring);


    int trackconfig = gettrack(conn, simdata->track);
    trackconfig = addtrackconfig(conn, trackconfig, simdata->track, simdata->trackdistancearound);
    int eventid = addevent(conn, trackconfig);
    int driverid = getdriver(conn, simdata->driver);
    driverid = adddriver(conn, driverid, simdata->driver);
    int carid = getcar(conn, simdata->car);
    carid = addcar(conn, carid, simdata->car);

    // ?? close last session
// sessions
// session_id | event_id | event_type | track_time | session_name | start_time | duration_min | elapsed_ms | laps | weather | air_temp | road_temp | start_grip | current_grip | is_finished | finish_time | last_activity | http_port

// stints
// session_stint_id | driver_id | team_member_id | session_id | car_id | game_car_id | laps | valid_laps | best_lap_id | is_finished | started_at | finished_at

// stint laps
// stint_lap_id | stint_id | sector_1 | sector_2 | sector_3 | grip | tyre | time | cuts | crashes | car_crashes | max_speed | avg_speed | finished_at

// telemetry
// lap_id | telemetry

    int pitstatus = 0;
    int sessionstatus = 0;
    int lastpitstatus = -1;
    int lastsessionstatus = 0;
    int lap = 0;
    int lastlap = 0;

    int stintid = 0;
    int stintlapid = 0;
    int sessionid = 0;
    sessionid = addsession(conn, eventid, simdata->session, simdata->airtemp, simdata->tracktemp);
    stintid = addstint(conn, sessionid, driverid, carid);
    stintlapid = addstintlap(conn, stintid);
    sessionstatus = simdata->session;
    lastsessionstatus = sessionstatus;
    lap = simdata->lap;
    lastlap = lap;
    double update_rate = DATA_UPDATE_RATE;
    struct pollfd mypoll = { STDIN_FILENO, POLLIN|POLLPRI };
    int go = true;
    char lastsimstatus = false;
    int tick = 0;


    slogt("spline %f", simdata->trackspline);
    int track_samples = simdata->trackspline / TRACK_SAMPLE_RATE;
    slogt("track samples %i", track_samples);


    int* speeddata = malloc(track_samples * sizeof(simdata->velocity));
    double* steerdata = malloc(track_samples * sizeof(simdata->steer));
    double* acceldata = malloc(track_samples * sizeof(simdata->gas));
    double* brakedata = malloc(track_samples * sizeof(simdata->brake));


    while (go == true && p->program_state >= 0)
    {

        slogt("tick %i", tick);
        slogt("pos %f", simdata->playerspline);
        int pos = (int) track_samples * simdata->playerspline;
        slogt("normpos %i", pos);
        steerdata[pos] = simdata->steer;
        acceldata[pos] = simdata->gas;
        speeddata[pos] = simdata->velocity;
        brakedata[pos] = simdata->brake;

        pitstatus = 0;
        sessionstatus = simdata->session;
        lap = simdata->lap;
        if (sessionstatus != lastsessionstatus)
        {
            sessionid = addsession(conn, eventid, simdata->session, simdata->airtemp, simdata->tracktemp);
            pitstatus = 1;
        }
        if (simdata->inpit == true)
        {
            pitstatus = 1;
        }
        if (pitstatus = 0 && pitstatus != lastpitstatus)
        {
            // close last stint
            stintid = addstint(conn, sessionid, driverid, carid);
        }
        if (lap != lastlap)
        {
            slogt("New lap detected");
            stintlapid = addstintlap(conn, stintid);
            int telemid = addtelemetry(conn, track_samples, stintlapid);
            int b = updatetelemetry(conn, telemid, track_samples*sizeof(double), "steer", steerdata);
            b = updatetelemetry(conn, telemid, track_samples*sizeof(double), "accel", acceldata);
            b = updatetelemetry(conn, telemid, track_samples*sizeof(double), "brake", brakedata);
            //print_bytes(acceldata, tick*sizeof(double));
            print_bytes(&acceldata[pos], sizeof(double));
            slogt("last accel %f on tick %i", acceldata[track_samples], track_samples);
            slogt("telemetry respone: %i", b);
            // close last stint lap and telemetry lap
            tick = 0;
        }
        lastpitstatus = pitstatus;
        lastsessionstatus = sessionstatus;
        lastlap = lap;
        tick++;

        poll(&mypoll, 1, 1000 / DATA_UPDATE_RATE);

    }


    h_close_db(conn);
    h_clean_connection(conn);
}
