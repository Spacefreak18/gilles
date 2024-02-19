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

#include "hoeldb.h"
#include "telemetry.h"
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

char* removeSpacesFromStr(char* string)
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
    struct tm* timeinfo;
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

        getSim(simdata, simmap, &p->simon, &p->sim);

        if (p->simon == true && simdata->simstatus > 1)
        {
            p->program_state = 1;
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
    while (go == true && simdata->simstatus > 1)
    {
        simdatamap(simdata, simmap, p->sim);

        wclear(win1);
        wclear(win2);
        wclear(win3);
        wclear(win4);

        {
            // window 1 car diagnostics

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

        {
            // window 2 session info

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

        {
            // window 3 basic timing and scoring

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
            //int minutes = simdata->lastlap/60000;
            //int seconds = simdata->lastlap/1000-(minutes*60);
            //int fraction = simdata->lastlap-(minutes*60000)-(seconds*1000);
            wattrset(win3, COLOR_PAIR(1));
            wprintw(win3, "   Last Lap: ");
            sprintf(lastlap, "%d:%02d:%02d\n", simdata->lastlap.minutes, simdata->lastlap.seconds, simdata->lastlap.fraction);
            wattrset(win3, COLOR_PAIR(2));
            waddstr(win3, lastlap);

            char bestlap[14];
            //minutes = simdata->bestlap/60000;
            //seconds = simdata->bestlap/1000-(minutes*60);
            //fraction = simdata->bestlap-(minutes*60000)-(seconds*1000);
            wattrset(win3, COLOR_PAIR(1));
            wprintw(win3, "   Best Lap: ");
            sprintf(bestlap, "%d:%02d:%02d\n", simdata->bestlap.minutes, simdata->bestlap.seconds, simdata->bestlap.fraction);
            wattrset(win3, COLOR_PAIR(2));
            waddstr(win3, bestlap);

            wattrset(win3, COLOR_PAIR(1));
        }

        {
            // window 4 live standings timing and scoring
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
                    {
                        break;
                    }
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

void* simviewmysql(void* thargs)
{
    Parameters* p = (Parameters*) thargs;
    SimData* simdata = p->simdata;
    SimMap* simmap = p->simmap;

    struct _h_result result;
    struct _h_connection* conn;
    conn = h_connect_pgsql(p->db_conn);

    if (conn == NULL)
    {
        slogf("Unable to connect to configured Gilles database. Are the parameters in the config correct? Is the user allowed to access from this address?");
        p->err = E_FAILED_DB_CONN;
        return 0;
    }
    slogi("Starting telemetry");

    int trackconfig = gettrack(conn, simdata->track);
    if (trackconfig == -1)
    {
        slogf("Problem performing select query. Does the db user have read permissions?");
        p->err = E_FAILED_DB_CONN;
        return 0;
    }

    trackconfig = addtrackconfig(conn, trackconfig, simdata->track, simdata->trackdistancearound);
    if (trackconfig == -1)
    {
        slogf("Problem performing insert query. Does the db user have write permissions?");
        p->err = E_FAILED_DB_CONN;
        return 0;
    }
    slogt("Detected track configuration id: %i", trackconfig);
    int eventid = addevent(conn, trackconfig);
    int driverid = getdriver(conn, simdata->driver);
    driverid = adddriver(conn, driverid, simdata->driver);
    int carid = getcar(conn, simdata->car);
    carid = addcar(conn, carid, simdata->car);

    // ?? close last session

    int pitstatus = simdata->inpit;
    int sessionstatus = 0;
    int lastpitstatus = pitstatus;
    int lastsessionstatus = 0;
    int lap = 0;
    int lastlap = 0;
    int sector = 0;
    int lastsector = 0;

    int stintid = 0;
    int stintlapid = 0;
    int sessionid = 0;
    sessionid = addsession(conn, eventid, carid, simdata->session, simdata->airtemp, simdata->tracktemp, simdata);
    stintid = addstint(conn, sessionid, driverid, carid, simdata);
    stintlapid = addstintlap(conn, stintid, simdata);
    sessionstatus = simdata->session;
    lastsessionstatus = sessionstatus;
    lap = simdata->lap;
    lastlap = lap;
    double update_rate = DATA_UPDATE_RATE;
    struct pollfd mypoll = { STDIN_FILENO, POLLIN|POLLPRI };
    int go = true;
    char lastsimstatus = false;
    int tick = 0;


    int track_samples = simdata->trackspline / TRACK_SAMPLE_RATE;
    slogt("track samples %i", track_samples);

    int stintlaps = 1;
    int validstintlaps = 0;
    bool validind = true;

    int* speeddata = malloc(track_samples * sizeof(simdata->velocity));
    int* rpmdata = malloc(track_samples * sizeof(simdata->rpms));
    int* geardata = malloc(track_samples * sizeof(simdata->gear));
    double* steerdata = malloc(track_samples * sizeof(simdata->steer));
    double* acceldata = malloc(track_samples * sizeof(simdata->gas));
    double* brakedata = malloc(track_samples * sizeof(simdata->brake));

    int sectortimes[4];

    if (p->program_state < 0)
    {
        go = false;
    }

    while (go == true)
    {

        slogt("tick %i", tick);
        int pos = (int) track_samples * simdata->playerspline;
        slogt("pos %f normpos %i of samples %i", simdata->playerspline, pos, track_samples);

        steerdata[pos] = simdata->steer;
        acceldata[pos] = simdata->gas;
        brakedata[pos] = simdata->brake;
        speeddata[pos] = simdata->velocity;
        rpmdata[pos] = simdata->rpms;
        geardata[pos] = simdata->gear;




        sessionstatus = simdata->session;
        lap = simdata->lap;
        sector = simdata->sectorindex;
        if (simdata->lapisvalid == false && validind == true)
        {
            validind = false;
        }
        sectortimes[simdata->sectorindex] = simdata->lastsectorinms;
        if (sessionstatus != lastsessionstatus)
        {
            closestint(conn, stintid, stintlaps, validstintlaps);
            closesession(conn, sessionid);
            if (sessionstatus > 1)
            {
                sessionid = addsession(conn, eventid, carid, simdata->session, simdata->airtemp, simdata->tracktemp, simdata);
            }

            //pitstatus = 1;
            stintlaps = 1;
            validstintlaps = 0;
        }
        pitstatus = simdata->inpit;
        if (simdata->inpit == true && pitstatus != lastpitstatus)
        {
            //pitstatus = 1;
            //}
            //if (pitstatus = 0 && pitstatus != lastpitstatus)
            //{
            // close last stint

            closestint(conn, stintid, stintlaps, validstintlaps);
            stintid = addstint(conn, sessionid, driverid, carid, simdata);
            stintlaps = 1;
            validstintlaps = 0;
        }
        if (lap != lastlap)
        {
            slogt("New lap detected");
            stintlaps++;
            if (validind == true)
            {
                validstintlaps++;
            }

            closelap(conn, stintlapid, sectortimes[1], sectortimes[2], simdata->lastsectorinms, 0, 0, 0, 0, simdata);

            stintlapid = addstintlap(conn, stintid, simdata);
            int telemid = addtelemetry(conn, track_samples, stintlapid);
            int b = updatetelemetrydata(conn, track_samples, telemid, stintlapid, speeddata, geardata, rpmdata, steerdata, acceldata, brakedata);
            tick = 0;
            // assume lap is valid until it isn't
            validind = true;
        }

        lastpitstatus = pitstatus;
        lastsessionstatus = sessionstatus;
        lastsector = sector;
        lastlap = lap;
        tick++;


        poll(&mypoll, 1, 1000 / DATA_UPDATE_RATE);

        if (p->program_state < 0)
        {
            int telemid = addtelemetry(conn, track_samples, stintlapid);
            int b = updatetelemetrydata(conn, track_samples, telemid, stintlapid, speeddata, geardata, rpmdata, steerdata, acceldata, brakedata);
            closelap(conn, stintlapid, sectortimes[1], sectortimes[2], simdata->lastsectorinms, 0, 0, 0, 0, simdata);
            closestint(conn, stintid, stintlaps, validstintlaps);
            closesession(conn, sessionid);
            go = false;
        }
    }


    h_close_db(conn);
    h_clean_connection(conn);
}
