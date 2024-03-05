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
    curs_set(0);

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

            int row = 2;
            int col1 = 3;
            int col2 = 7;
            char speed[5];
            wbkgd(win1,COLOR_PAIR(1));
            wattrset(win1, COLOR_PAIR(1));
            snprintf(speed, 5, "%i", simdata->velocity);
            mvwaddnstr(win1, row, col1, "Speed:", -1);
            wattrset(win1, COLOR_PAIR(2));
            mvwaddnstr(win1, row, col1+col2, speed, -1);
            row++;

            char rpm[6];
            wbkgd(win1,COLOR_PAIR(1));
            wattrset(win1, COLOR_PAIR(1));
            snprintf(rpm, 6, "%i", simdata->rpms);
            mvwaddnstr(win1, row, col1, "RPM:", -1);
            wattrset(win1, COLOR_PAIR(2));
            mvwaddnstr(win1, row, col1+col2, rpm, -1);
            row++;

            char gear[2];
            wbkgd(win1,COLOR_PAIR(1));
            wattrset(win1, COLOR_PAIR(1));
            snprintf(gear, 2, "%i", simdata->gear);
            mvwaddnstr(win1, row, col1, "Gear:", -1);
            wattrset(win1, COLOR_PAIR(2));
            mvwaddnstr(win1, row, col1+col2, gear, -1);
            row++;

            char gas[14];
            wattrset(win1, COLOR_PAIR(1));
            snprintf(gas, 14, "%f", simdata->gear);
            mvwaddnstr(win1, row, col1, "Gas:", -1);
            wattrset(win1, COLOR_PAIR(2));
            mvwaddnstr(win1, row, col1+col2, gas, -1);
            row++;

            char brake[14];
            wattrset(win1, COLOR_PAIR(1));
            snprintf(brake, 14, "%f", simdata->brake);
            mvwaddnstr(win1, row, col1, "Brake:", -1);
            wattrset(win1, COLOR_PAIR(2));
            mvwaddnstr(win1, row, col1+col2, brake, -1);
            row++;

            char fuel[14];
            wattrset(win1, COLOR_PAIR(1));
            snprintf(fuel, 14, "%f", simdata->fuel);
            mvwaddnstr(win1, row, col1, "Fuel: ", -1);
            wattrset(win1, COLOR_PAIR(2));
            mvwaddnstr(win1, row, col1+col2, fuel, -1);
            row++;
            row+=3;

            // setup tyre and brake data strings
            char braketemp0[14];
            snprintf(braketemp0, 14, "%.0f", simdata->braketemp[0]);
            char braketemp1[14];
            snprintf(braketemp1, 14, "%.0f", simdata->braketemp[1]);
            char braketemp2[14];
            snprintf(braketemp2, 14, "%.0f", simdata->braketemp[2]);
            char braketemp3[14];
            snprintf(braketemp3, 14, "%.0f", simdata->braketemp[3]);

            char tyretemp0[14];
            snprintf(tyretemp0, 14, "%.0f", simdata->tyretemp[0]);
            char tyretemp1[14];
            snprintf(tyretemp1, 14, "%.0f", simdata->tyretemp[1]);
            char tyretemp2[14];
            snprintf(tyretemp2, 14, "%.0f", simdata->tyretemp[2]);
            char tyretemp3[14];
            snprintf(tyretemp3, 14, "%.0f", simdata->tyretemp[3]);

            char tyrepressure0[14];
            snprintf(tyrepressure0, 14, "%.0f", simdata->tyrepressure[0]);
            char tyrepressure1[14];
            snprintf(tyrepressure1, 14, "%.0f", simdata->tyrepressure[1]);
            char tyrepressure2[14];
            snprintf(tyrepressure2, 14, "%.0f", simdata->tyrepressure[2]);
            char tyrepressure3[14];
            snprintf(tyrepressure3, 14, "%.0f", simdata->tyrepressure[3]);

            char tyrewear0[14];
            snprintf(tyrewear0, 14, "%.0f", simdata->tyrewear[0]);
            char tyrewear1[14];
            snprintf(tyrewear1, 14, "%.0f", simdata->tyrewear[1]);
            char tyrewear2[14];
            snprintf(tyrewear2, 14, "%.0f", simdata->tyrewear[2]);
            char tyrewear3[14];
            snprintf(tyrewear3, 14, "%.0f", simdata->tyrewear[3]);

            {
                int row = 11;
                int col1 = 2;
                int col2 = 12;
                int col3 = 8;
                int col4 = 12;
                // front brakes

                wattrset(win1, COLOR_PAIR(1));
                mvwaddnstr(win1, row, col1, "braketemp:", -1);
                wattrset(win1, COLOR_PAIR(2));
                mvwaddnstr(win1, row, col1+col2, braketemp0, -1);
                wattrset(win1, COLOR_PAIR(1));
                mvwaddnstr(win1, row, col1+col2+col3, "braketemp:", -1);
                wattrset(win1, COLOR_PAIR(2));
                mvwaddnstr(win1, row, col1+col2+col3+col4, braketemp1, -1);
                row++;


                // front tyres
                col2=7;
                col3= 16;
                col4=7;

                row++;
                wattrset(win1, COLOR_PAIR(1));
                mvwaddnstr(win1, row, col1, "temp:", -1);
                wattrset(win1, COLOR_PAIR(2));
                mvwaddnstr(win1, row, col1+col2, tyretemp0, -1);

                wattrset(win1, COLOR_PAIR(1));
                mvwaddnstr(win1, row, col1+col2+col3, "temp:", -1);
                wattrset(win1, COLOR_PAIR(2));
                mvwaddnstr(win1, row, col1+col2+col3+col4, tyretemp1, -1);
                row++;

                wattrset(win1, COLOR_PAIR(1));
                mvwaddnstr(win1, row, col1, "pres:", -1);
                wattrset(win1, COLOR_PAIR(2));
                mvwaddnstr(win1, row, col1+col2, tyrepressure0, -1);

                wattrset(win1, COLOR_PAIR(1));
                mvwaddnstr(win1, row, col1+col2+col3, "pres:", -1);
                wattrset(win1, COLOR_PAIR(2));
                mvwaddnstr(win1, row, col1+col2+col3+col4, tyrepressure1, -1);
                row++;

                wattrset(win1, COLOR_PAIR(1));
                mvwaddnstr(win1, row, col1, "wear:", -1);
                wattrset(win1, COLOR_PAIR(2));
                mvwaddnstr(win1, row, col1+col2, tyrewear0, -1);

                wattrset(win1, COLOR_PAIR(1));
                mvwaddnstr(win1, row, col1+col2+col3, "wear:", -1);
                wattrset(win1, COLOR_PAIR(2));
                mvwaddnstr(win1, row, col1+col2+col3+col4, tyrewear1, -1);
                row++;


                // rear tyres

                row+=3;
                wattrset(win1, COLOR_PAIR(1));
                mvwaddnstr(win1, row, col1, "temp:", -1);
                wattrset(win1, COLOR_PAIR(2));
                mvwaddnstr(win1, row, col1+col2, tyretemp2, -1);

                wattrset(win1, COLOR_PAIR(1));
                mvwaddnstr(win1, row, col1+col2+col3, "temp:", -1);
                wattrset(win1, COLOR_PAIR(2));
                mvwaddnstr(win1, row, col1+col2+col3+col4, tyretemp3, -1);
                row++;

                wattrset(win1, COLOR_PAIR(1));
                mvwaddnstr(win1, row, col1, "pres:", -1);
                wattrset(win1, COLOR_PAIR(2));
                mvwaddnstr(win1, row, col1+col2, tyrepressure2, -1);

                wattrset(win1, COLOR_PAIR(1));
                mvwaddnstr(win1, row, col1+col2+col3, "pres:", -1);
                wattrset(win1, COLOR_PAIR(2));
                mvwaddnstr(win1, row, col1+col2+col3+col4, tyrepressure3, -1);
                row++;

                wattrset(win1, COLOR_PAIR(1));
                mvwaddnstr(win1, row, col1, "wear:", -1);
                wattrset(win1, COLOR_PAIR(2));
                mvwaddnstr(win1, row, col1+col2, tyrewear2, -1);

                wattrset(win1, COLOR_PAIR(1));
                mvwaddnstr(win1, row, col1+col2+col3, "wear:", -1);
                wattrset(win1, COLOR_PAIR(2));
                mvwaddnstr(win1, row, col1+col2+col3+col4, tyrewear3, -1);
                row++;

                // rear brakes
                row++;
                col2 = 12;
                col3 = 8;
                col4 = 12;
                wattrset(win1, COLOR_PAIR(1));
                mvwaddnstr(win1, row, col1, "braketemp:", -1);
                wattrset(win1, COLOR_PAIR(2));
                mvwaddnstr(win1, row, col1+col2, braketemp2, -1);
                wattrset(win1, COLOR_PAIR(1));
                mvwaddnstr(win1, row, col1+col2+col3, "braketemp:", -1);
                wattrset(win1, COLOR_PAIR(2));
                mvwaddnstr(win1, row, col1+col2+col3+col4, braketemp3, -1);
                row++;

                // draw boxes to represent the tyres
                wattrset(win1, COLOR_PAIR(1));
                rectangle(12, 13, 16, 17);  // left front
                rectangle(12, 19, 16, 23); // right front
                rectangle(18, 13, 22, 17);  // left rear
                rectangle(18, 19, 22, 23); // right rear
            }
        }

        {
            // window 2 session info

            int row = 1;
            int col1gap = 3;
            int col2gap = 14;

            char airtemp[6];
            wbkgd(win2, COLOR_PAIR(1));
            wattrset(win2, COLOR_PAIR(1));
            snprintf(airtemp, 6, "%.0f", simdata->airtemp);
            mvwaddnstr(win2, row, col1gap, "Air Temp:", -1);
            wattrset(win2, COLOR_PAIR(2));
            mvwaddnstr(win2, row, col1gap+col2gap, airtemp, -1);
            row++;

            char airdensity[6];
            wbkgd(win2, COLOR_PAIR(1));
            wattrset(win2, COLOR_PAIR(1));
            snprintf(airdensity, 6, "%.0f", simdata->airdensity);
            mvwaddnstr(win2, row, col1gap, "Humidity:", -1);
            wattrset(win2, COLOR_PAIR(2));
            mvwaddnstr(win2, row, col1gap+col2gap, airdensity, -1);
            row++;

            char tracktemp[6];
            wbkgd(win2, COLOR_PAIR(1));
            wattrset(win2, COLOR_PAIR(1));
            snprintf(tracktemp, 6, "%.0f", simdata->tracktemp);
            mvwaddnstr(win2, row, col1gap, "Track Temp:", -1);
            wattrset(win2, COLOR_PAIR(2));
            mvwaddnstr(win2, row, col1gap+col2gap, tracktemp, -1);
            row++;

            char numlaps[5];
            wbkgd(win2, COLOR_PAIR(1));
            wattrset(win2, COLOR_PAIR(1));
            snprintf(numlaps, 6, "%.0f", simdata->numlaps);
            mvwaddnstr(win2, row, col1gap, "Laps:", -1);
            wattrset(win2, COLOR_PAIR(2));
            mvwaddnstr(win2, row, col1gap+col2gap, numlaps, -1);
            row++;


            char timeleft[14];
            int hours = simdata->timeleft/6000;
            int minutes = simdata->timeleft/60 - (hours*6000);
            int seconds = simdata->timeleft-((minutes*60)+(hours*6000));
            //int fraction = simdata->timeleft-(minutes*60000)-(seconds*1000);
            wbkgd(win2, COLOR_PAIR(1));
            wattrset(win2, COLOR_PAIR(1));
            mvwaddnstr(win2, row, col1gap, "Time Left:", -1);
            snprintf(timeleft, 14, "%02d:%02d:%02d\n", hours, minutes, seconds);
            wattrset(win2, COLOR_PAIR(2));
            mvwaddnstr(win2, row, col1gap+col2gap, timeleft, -1);
            row++;

            char currenttime[14];
            hours = simdata->time/6000;
            minutes = simdata->time/60 - (hours*6000);
            seconds = simdata->time-((minutes*60)+(hours*6000));
            //fraction = simdata->time-(minutes*60000)-(seconds*1000);
            wbkgd(win2, COLOR_PAIR(1));
            wattrset(win2, COLOR_PAIR(1));
            mvwaddnstr(win2, row, col1gap, "Current Time:", -1);
            snprintf(currenttime, 14, "%02d:%02d:%02d\n", hours, minutes, seconds);
            wattrset(win2, COLOR_PAIR(2));
            mvwaddnstr(win2, row, col1gap+col2gap, currenttime, -1);

            wattrset(win2, COLOR_PAIR(1));
        }

        {
            // window 3 basic timing and scoring

            int row = 1;
            int col1gap = 3;
            int col2gap = 14;

            char car[14];
            wbkgd(win3, COLOR_PAIR(1));
            wattrset(win3, COLOR_PAIR(1));
            snprintf(car, 14, "%s", simdata->car);
            mvwaddnstr(win3, row, col1gap, "Car:", -1);
            wattrset(win3, COLOR_PAIR(2));
            mvwaddnstr(win3, row, col1gap+col2gap, car, -1);
            row++;

            char track[14];
            wbkgd(win3, COLOR_PAIR(1));
            wattrset(win3, COLOR_PAIR(1));
            snprintf(track, 14, "%s", simdata->track);
            mvwaddnstr(win3, row, col1gap, "Track:", -1);
            wattrset(win3, COLOR_PAIR(2));
            mvwaddnstr(win3, row, col1gap+col2gap, track, -1);
            row++;

            char driver[14];
            wbkgd(win3, COLOR_PAIR(1));
            wattrset(win3, COLOR_PAIR(1));
            snprintf(driver, 14, "%s", simdata->driver);
            mvwaddnstr(win3, row, col1gap, "Driver:", -1);
            wattrset(win3, COLOR_PAIR(2));
            mvwaddnstr(win3, row, col1gap+col2gap, driver, -1);
            row++;

            char lap[4];
            wbkgd(win3, COLOR_PAIR(1));
            wattrset(win3, COLOR_PAIR(1));
            snprintf(lap, 4, "%i", simdata->lap);
            mvwaddnstr(win3, row, col1gap, "Lap:", -1);
            wattrset(win3, COLOR_PAIR(2));
            mvwaddnstr(win3, row, col1gap+col2gap, lap, -1);
            row++;

            char position[4];
            wbkgd(win3, COLOR_PAIR(1));
            wattrset(win3, COLOR_PAIR(1));
            snprintf(position, 4, "%i", simdata->position);
            mvwaddnstr(win3, row, col1gap, "Position:", -1);
            wattrset(win3, COLOR_PAIR(2));
            mvwaddnstr(win3, row, col1gap+col2gap, position, -1);
            row++;

            char lastlap[14];
            wattrset(win3, COLOR_PAIR(1));
            snprintf(lastlap, 14, "%d:%02d:%02d\n", simdata->lastlap.minutes, simdata->lastlap.seconds, simdata->lastlap.fraction);
            mvwaddnstr(win3, row, col1gap, "Last Lap:", -1);
            wattrset(win3, COLOR_PAIR(2));
            mvwaddnstr(win3, row, col1gap+col2gap, lastlap, -1);
            row++;

            char bestlap[14];
            wattrset(win3, COLOR_PAIR(1));
            snprintf(bestlap, 14, "%d:%02d:%02d\n", simdata->bestlap.minutes, simdata->bestlap.seconds, simdata->bestlap.fraction);
            mvwaddnstr(win3, row, col1gap, "Best Lap:", -1);
            wattrset(win3, COLOR_PAIR(2));
            mvwaddnstr(win3, row, col1gap+col2gap, bestlap, -1);

            wattrset(win3, COLOR_PAIR(1));
        }

        {
            // window 4 live standings timing and scoring

            int row = 1;
            wattrset(win4, COLOR_PAIR(4));
            mvwaddnstr(win4, row, 3, "P", -1);
            mvwaddnstr(win4, row, 10, "Driver", -1);
            mvwaddnstr(win4, row, 35, "Car", -1);
            mvwaddnstr(win4, row, 48, "Laps", -1);
            mvwaddnstr(win4, row, 58, "Last", -1);
            mvwaddnstr(win4, row, 71, "Best", -1);
            mvwaddnstr(win4, row, 82, "Pit", -1);
            row++;

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
                char pos[4];
                char car[maxstrlen];
                char driver[maxstrlen];
                char lap[4];

                snprintf(pos, 4, "%02d", simdata->cars[i].pos);
                snprintf(driver, maxstrlen, "%-*.*s", maxstrlen, maxstrlen, simdata->cars[i].driver);
                snprintf(car, maxstrlen, "%-*.*s", maxstrlen, maxstrlen, simdata->cars[i].car);
                snprintf(lap, 4, "%i", simdata->cars[i].lap);

                char clastlap[14];
                int lastlap = simdata->cars[i].lastlap;
                int minutes = lastlap/60000;
                int seconds = lastlap/1000-(minutes*60);
                int fraction = lastlap-(minutes*60000)-(seconds*1000);
                snprintf(clastlap, 14, "%02d:%02d:%03d", minutes, seconds, fraction);

                char cbestlap[14];
                int bestlap = simdata->cars[i].bestlap;
                minutes = bestlap/60000;
                seconds = bestlap/1000-(minutes*60);
                fraction = bestlap-(minutes*60000)-(seconds*1000);
                snprintf(cbestlap, 14, "%02d:%02d:%03d", minutes, seconds, fraction);

                mvwaddnstr(win4, row, 2, pos, -1);
                mvwaddnstr(win4, row, 7, driver, -1);
                mvwaddnstr(win4, row, 27, car, -1);
                mvwaddnstr(win4, row, 50, lap, -1);
                mvwaddnstr(win4, row, 56, clastlap, -1);
                mvwaddnstr(win4, row, 69, cbestlap, -1);

                char pitstatus[8];
                if(simdata->cars[i].inpitlane == 0 && simdata->cars[i].inpit == 0)
                {
                    wattrset(win4, COLOR_PAIR(1));
                    snprintf(pitstatus, 8, "%s", "ontrack");
                }
                else
                {
                    if(simdata->cars[i].inpit > 0)
                    {
                        snprintf(pitstatus, 8, "%s", "pit");
                    }
                    else
                    {
                        snprintf(pitstatus, 8, "%s", "pitlane");
                    }
                }
                mvwaddnstr(win4, row, 80, pitstatus, -1);
                row++;

                i = ihold;
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

    int* speeddata = calloc(track_samples, sizeof(simdata->velocity));
    int* rpmdata = calloc(track_samples, sizeof(simdata->rpms));
    int* geardata = calloc(track_samples, sizeof(simdata->gear));
    double* steerdata = calloc(track_samples, sizeof(simdata->steer));
    double* acceldata = calloc(track_samples, sizeof(simdata->gas));
    double* brakedata = calloc(track_samples, sizeof(simdata->brake));

    int sectortimes[4];

    if (p->program_state < 0)
    {
        go = false;
    }

    int lastpos = 0;
    while (go == true)
    {

        int pos = (int) track_samples * simdata->playerspline;
        slogt("tick %i pos %f normpos %i of samples %i", tick, simdata->playerspline, pos, track_samples);

        steerdata[pos] = simdata->steer;
        acceldata[pos] = simdata->gas;
        brakedata[pos] = simdata->brake;
        speeddata[pos] = simdata->velocity;
        rpmdata[pos] = simdata->rpms;
        geardata[pos] = simdata->gear;

        if(lastpos < pos-1)
        {
            while(lastpos < pos)
            {
                steerdata[lastpos] = simdata->steer;
                acceldata[lastpos] = simdata->gas;
                brakedata[lastpos] = simdata->brake;
                speeddata[lastpos] = simdata->velocity;
                rpmdata[lastpos] = simdata->rpms;
                geardata[lastpos] = simdata->gear;
                lastpos++;
            }
        }
        lastpos = pos;

        slogt("speed %i rpms %i gear %i steer %f gas %f brake %f", speeddata[pos], rpmdata[pos], geardata[pos], steerdata[pos],
                acceldata[pos], brakedata[pos]);

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
            int b = updatetelemetrydata(conn, track_samples, telemid, stintlapid, speeddata, rpmdata, geardata, steerdata, acceldata, brakedata);
            tick = 0;
            // assume lap is valid until it isn't
            validind = true;
            speeddata = calloc(track_samples, sizeof(simdata->velocity));
            rpmdata = calloc(track_samples, sizeof(simdata->rpms));
            geardata = calloc(track_samples, sizeof(simdata->gear));
            steerdata = calloc(track_samples, sizeof(simdata->steer));
            acceldata = calloc(track_samples, sizeof(simdata->gas));
            brakedata = calloc(track_samples, sizeof(simdata->brake));
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
            int b = updatetelemetrydata(conn, track_samples, telemid, stintlapid, speeddata, rpmdata, geardata, steerdata, acceldata, brakedata);
            closelap(conn, stintlapid, sectortimes[1], sectortimes[2], simdata->lastsectorinms, 0, 0, 0, 0, simdata);
            closestint(conn, stintid, stintlaps, validstintlaps);
            closesession(conn, sessionid);
            go = false;
        }
    }


    h_close_db(conn);
    h_clean_connection(conn);
}
