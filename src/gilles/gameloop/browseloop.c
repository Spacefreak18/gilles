#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
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

#include "session.h"
#include "gameloop.h"
#include "../helper/parameters.h"
#include "../helper/confighelper.h"
#include "../helper/dirhelper.h"
#include "../simulatorapi/simapi/simapi/simdata.h"
#include "../simulatorapi/simapi/simapi/simmapper.h"
#include "../slog/slog.h"

#define DEFAULT_UPDATE_RATE      60
#define DATA_UPDATE_RATE         4


#define SESSIONS_SCREEN     1
#define STINTS_SCREEN       2
#define LAPS_SCREEN         3

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
    //win23y = winy/3;
    //win23x = winx/3;
    //win1 = newwin(winx,winy,0,0);
    //win2 = newwin(win23x,win23y,1,win23y-1);
    //win3 = newwin(win23x,win23y,1,win23y*2-1);
    //win4 = newwin(winx-win23x-2,winy-win23y,win23x+1,win23y-1);
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
    //win23y = winy/3;
    //win23x = winx/3;
    //win2 = newwin(win23x,win23y,1,win23y-1);
    //win3 = newwin(win23x,win23y,1,win23y*2-1);
    //win4 = newwin(winx-win23x-2,winy-win23y,win23x+1,win23y-1);

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

void print_result(struct _h_result result) {
  int col, row, i;
  char buf[64];
  slogi("rows: %d, col: %d", result.nb_rows, result.nb_columns);
  for (row = 0; row<result.nb_rows; row++) {
    for (col=0; col<result.nb_columns; col++) {
      switch(result.data[row][col].type) {
        case HOEL_COL_TYPE_INT:
          printf("| %d ", ((struct _h_type_int *)result.data[row][col].t_data)->value);
          break;
        case HOEL_COL_TYPE_DOUBLE:
          printf("| %f ", ((struct _h_type_double *)result.data[row][col].t_data)->value);
          break;
        case HOEL_COL_TYPE_TEXT:
          printf("| %s ", ((struct _h_type_text *)result.data[row][col].t_data)->value);
          break;
        case HOEL_COL_TYPE_BLOB:
          for (i=0; i<((struct _h_type_blob *)result.data[row][col].t_data)->length; i++) {
            printf("%c", *((char*)(((struct _h_type_blob *)result.data[row][col].t_data)->value+i)));
            if (i%80 == 0 && i>0) {
              printf("\n");
            }
          }
          break;
        case HOEL_COL_TYPE_DATE:
          strftime(buf, 64, "%Y-%m-%d %H:%M:%S", &((struct _h_type_datetime *)result.data[row][col].t_data)->value);
          printf("| %s ", buf);
        case HOEL_COL_TYPE_NULL:
          printf("| [null] ");
          break;
      }
    }
    printf("|\n");
  }
}

void get_row_results(struct _h_result result, DBField* fields, void* rows, size_t rowsize) {
  int col, row, i;
  char buf[64];
  //sess->hasdata = true;
  //sess->rows = result.nb_rows;
  slogi("rows: %d, col: %d", result.nb_rows, result.nb_columns);
  char* aaa = (char *) rows;
  for (row = 0; row<result.nb_rows; row++) {

    char* aa = (char*)aaa + (rowsize * row);
    for (col=0; col<result.nb_columns; col++) {
      char* a = (char*) aa + fields[col].offset; 
      switch(result.data[row][col].type) {
        case HOEL_COL_TYPE_INT:
          int bb = ((struct _h_type_int *)result.data[row][col].t_data)->value;
          *(int*) a = bb;
          break;
        case HOEL_COL_TYPE_DOUBLE:
          double cc = ((struct _h_type_double *)result.data[row][col].t_data)->value;
          *(double*) a = cc;
          break;
        case HOEL_COL_TYPE_TEXT:
          char* ddd = ((struct _h_type_text *)result.data[row][col].t_data)->value;
          memcpy(a, ddd, fields[col].size);
          break;
        case HOEL_COL_TYPE_BLOB:
          for (i=0; i<((struct _h_type_blob *)result.data[row][col].t_data)->length; i++) {
            printf("%c", *((char*)(((struct _h_type_blob *)result.data[row][col].t_data)->value+i)));
            if (i%80 == 0 && i>0) {
              printf("\n");
            }
          }
          break;
        case HOEL_COL_TYPE_DATE:
          strftime(a, fields[col].size, "%Y-%m-%d %H:%M:%S", &((struct _h_type_datetime *)result.data[row][col].t_data)->value);
          //strftime(buf, 64, "%Y-%m-%d %H:%M:%S", &((struct _h_type_datetime *)result.data[row][col].t_data)->value);
          printf("| %s ", buf);
        case HOEL_COL_TYPE_NULL:
          printf("| [null] ");
          break;
      }
    }
    printf("|\n");
  }
}


int getsessions(struct _h_connection* conn, const char* sessionname, SessionDbo* sess)
{
    struct _h_result result;
    struct _h_data * data;
    char* query = malloc(99 * sizeof(char));
    slogt("Performing query");

    //sprintf(query, "select session_id, event_id, event_type, duration_min, elapsed_ms, laps, air_temp, road_temp, start_grip, current_grip, is_finished, http_port from %s", "Sessions");
    sprintf(query, "select session_id, event_id, event_type, laps FROM %s", "Sessions");
    if (h_query_select(conn, query, &result) == H_OK) {
        sess->rows = malloc(sizeof(SessionRowData) * result.nb_rows);
        get_row_results(result, sess->fields, sess->rows, sizeof(SessionRowData));
        //get_session_result(result, sess);
        h_clean_result(&result);
    } else {
        printf("Error executing query\n");
    }
    free(query);

    return result.nb_rows;
}

int getstints(struct _h_connection* conn, const char* sessionname, StintDbo* stint, int use_id)
{
    struct _h_result result;
    struct _h_data * data;
    char* query = malloc(150 * sizeof(char));
    slogt("Performing query stints");


    sprintf(query, "select stint_id, driver_id, team_member_id, session_id, car_id, game_car_id, laps, valid_laps, best_lap_id FROM %s WHERE session_id=%i", "Stints", use_id);
    if (h_query_select(conn, query, &result) == H_OK) {
        stint->rows = malloc(sizeof(StintRowData) * result.nb_rows);
        get_row_results(result, stint->fields, stint->rows, sizeof(StintRowData));
        //get_stint_result(result, stint);
        h_clean_result(&result);
    } else {
        printf("Error executing query\n");
    }
    free(query);

    return result.nb_rows;
}

int getlaps(struct _h_connection* conn, const char* sessionname, LapDbo* laps, int use_id)
{
    struct _h_result result;
    struct _h_data * data;
    char* query = malloc(150 * sizeof(char));
    slogt("Performing query laps");


    sprintf(query, "select * FROM %s WHERE stint_id=%i", "Laps", use_id);
    if (h_query_select(conn, query, &result) == H_OK) {
        laps->rows = malloc(sizeof(LapRowData) * result.nb_rows);
        get_row_results(result, laps->fields, laps->rows, sizeof(LapRowData));
        //get_stint_result(result, stint);
        h_clean_result(&result);
    } else {
        printf("Error executing query\n");
    }
    free(query);

    return result.nb_rows;
}

//int getsessions(struct _h_connection* conn, const char* carname)
//{
//    json_t *j_result;
//    char* where_clause = h_build_where_clause(conn, "car_name=%s", carname);
//    json_t* j_query = json_pack("{sss[s]s{s{ssss}}}","table", "sessions", "columns", "session_id", "session_name", "laps",
//            "where", " ", "operator", "raw",
//            "value", where_clause);
//
//    slogi("Looking for car named %s", carname);
//    //char* qq;
//    int res = h_select(conn, j_query, &j_result, NULL);
//    //slogi("here your query: %s", qq);
//  // Deallocate j_query since it won't be needed anymore
//  json_decref(j_query);
//  h_free(where_clause);
//  int session_id = -1;
//  int sessions = 0;
//  // Test query execution result
//  if (res == H_OK) {
//    // Print result
//    char* dump = json_dumps(j_result, JSON_INDENT(2));
//    slogi("json select result is\n%s", dump);
//    int index1 = json_array_size(j_result);
//    sessions = index1;
//    if (index1 == 0)
//    {
//        slogw("no car by this name");
//    }
//    else {
//        for (int k = 0; k < index1; k++)
//        {
//            json_t* jj = json_array_get(j_result, k);
//            session_id = json_integer_value(json_object_get(jj, "session_id"));
//            int laps = json_integer_value(json_object_get(jj, "laps"));
//            slogt("found session %i with %i laps", session_id, laps);
//        }
//    }
//    // Deallocate data result
//    json_decref(j_result);
//    free(dump);
//  } else {
//    sloge("Error executing select query: %d", res);
//  }
//  return sessions;
//}





void* browseloop(Parameters* p)
{

    struct _h_result result;
    struct _h_connection * conn;
    char* connectionstring = "host=zorak.brak dbname=gilles user=test password=thisisatest";
    conn = h_connect_pgsql(connectionstring);

    slogt("Starting analyzer");
    curses_init();

    timeout(DEFAULT_UPDATE_RATE);

    SessionDbo sess;
    DBField sessid;
    sessid.type = HOEL_COL_TYPE_INT;
    sessid.offset = 0;
    sessid.colnum = 0;
    DBField laps;
    laps.type = HOEL_COL_TYPE_INT;
    laps.offset = sizeof(int);
    laps.offset = offsetof(SessionRowData, laps);
    laps.colnum = 1;
    DBField eventid;
    eventid.type = HOEL_COL_TYPE_INT;
    eventid.offset = offsetof(SessionRowData, event_id);
    DBField eventtype;
    eventtype.type = HOEL_COL_TYPE_INT;
    eventtype.offset = offsetof(SessionRowData, event_type);

    //SessionFields sf;
    sess.fields[0] = sessid;
    sess.fields[1] = eventid;
    sess.fields[2] = eventtype;
    sess.fields[3] = laps;
    //sf.session_id = sessid;
    //sf.laps = laps;

    //sess.fields = sf;



    int action = 0;
    int selection = 1;
    int lastselection = 1;
    int selection1 = 0;
    int selection2 = 0;
    int sessions = 0;
    int lapsresults = 0;
    int curresults = 0;
    int stintsid = 0;

    StintDbo stints;
    DBField stintid;
    stintid.type = HOEL_COL_TYPE_INT;
    stintid.offset = 0;
    DBField driverid;
    driverid.type = HOEL_COL_TYPE_INT;
    driverid.offset = offsetof(StintRowData, driver_id);
    DBField teammemberid;
    teammemberid.type = HOEL_COL_TYPE_INT;
    teammemberid.offset = offsetof(StintRowData, team_member_id);
    DBField sessionidstint;
    sessionidstint.type = HOEL_COL_TYPE_INT;
    sessionidstint.offset = offsetof(StintRowData, session_id);
    DBField carid;
    carid.type = HOEL_COL_TYPE_INT;
    carid.offset = offsetof(StintRowData, car_id);
    DBField gamecarid;
    gamecarid.type = HOEL_COL_TYPE_INT;
    gamecarid.offset = offsetof(StintRowData, game_car_id);
    DBField stintlaps;
    stintlaps.type = HOEL_COL_TYPE_INT;
    stintlaps.offset = offsetof(StintRowData, laps);
    DBField validlaps;
    validlaps.type = HOEL_COL_TYPE_INT;
    validlaps.offset = offsetof(StintRowData, valid_laps);
    DBField bestlapid;
    bestlapid.type = HOEL_COL_TYPE_INT;
    bestlapid.offset = offsetof(StintRowData, best_lap_id);

    stints.fields[0] = stintid;
    stints.fields[1] = driverid;
    stints.fields[2] = teammemberid;
    stints.fields[3] = sessionidstint;
    stints.fields[4] = carid;
    stints.fields[5] = gamecarid;
    stints.fields[6] = stintlaps;
    stints.fields[7] = validlaps;
    stints.fields[8] = bestlapid;

    LapDbo lapsdb;
    DBField lapsdbid;
    lapsdbid.type = HOEL_COL_TYPE_INT;
    lapsdbid.offset = 0;
    DBField lapsdbstintid;
    lapsdbstintid.type = HOEL_COL_TYPE_INT;
    lapsdbstintid.offset = offsetof(LapRowData, stint_id);
    DBField sector_1;
    sector_1.type = HOEL_COL_TYPE_INT;
    sector_1.offset = offsetof(LapRowData, sector_1);
    DBField sector_2;
    sector_2.type = HOEL_COL_TYPE_INT;
    sector_2.offset = offsetof(LapRowData, sector_2);
    DBField sector_3;
    sector_3.type = HOEL_COL_TYPE_INT;
    sector_3.offset = offsetof(LapRowData, sector_3);
    DBField grip;
    grip.type = HOEL_COL_TYPE_DOUBLE;
    grip.offset = offsetof(LapRowData, grip);
    DBField tyrec;
    tyrec.type = HOEL_COL_TYPE_TEXT;
    tyrec.offset = offsetof(LapRowData, tyre);
    tyrec.size = sizeof(unsigned char)*10;
    DBField time;
    time.type = HOEL_COL_TYPE_INT;
    time.offset = offsetof(LapRowData, time);
    DBField cuts;
    cuts.type = HOEL_COL_TYPE_INT;
    cuts.offset = offsetof(LapRowData, cuts);
    DBField carcrashes;
    carcrashes.type = HOEL_COL_TYPE_INT;
    carcrashes.offset = offsetof(LapRowData, crashes);
    DBField maxspeed;
    maxspeed.type = HOEL_COL_TYPE_INT;
    maxspeed.offset = offsetof(LapRowData, max_speed);
    DBField avgspeed;
    avgspeed.type = HOEL_COL_TYPE_INT;
    avgspeed.offset = offsetof(LapRowData, avg_speed);
    DBField lapsdbfinishedat;
    lapsdbfinishedat.type = HOEL_COL_TYPE_DATE;
    lapsdbfinishedat.offset = offsetof(LapRowData, finished_at);
    lapsdbfinishedat.size = sizeof(PDBTimeStamp);

    lapsdb.fields[0] = lapsdbid;
    lapsdb.fields[1] = lapsdbstintid;
    lapsdb.fields[2] = sector_1;
    lapsdb.fields[3] = sector_2;
    lapsdb.fields[4] = sector_3;
    lapsdb.fields[5] = grip;
    lapsdb.fields[6] = tyrec;
    lapsdb.fields[7] = time;
    lapsdb.fields[8] = cuts;
    lapsdb.fields[9] = carcrashes;
    lapsdb.fields[10] = maxspeed;
    lapsdb.fields[11] = avgspeed;
    lapsdb.fields[12] = lapsdbfinishedat;

    //slogt("sessions has %i rows", sess.numrows);

    action = 2;


    int go = true;
    char lastsimstatus = false;

    int screen = SESSIONS_SCREEN;
    char ch;
    box(win1, 0, 0);
    wrefresh(win1);
    int useid = 0;
    while (go == true)
    {

        if (lastselection != selection)
        {
            action = 1;
            lastselection = selection;
        }

        if (action == 2)
        {
                slogt("going to perform an action");
                sessions = getsessions(conn, "Sessions", &sess);
                curresults = sessions;
        }

        if (action == 3)
        {
                slogt("going to perform an action");

                stintsid = getstints(conn, "Stints", &stints, useid);
                curresults = stintsid;
        }
        if (action == 4)
        {
                slogt("going to perform an action");

                lapsresults = getlaps(conn, "laps", &lapsdb, useid);
                curresults = lapsresults;
        }

        if (action > 0)
        {
            wclear(win1);

            wprintw(win1, "\n");
            switch(screen) {
            case SESSIONS_SCREEN:
              for(int i=0; i<sessions+1; i++)
              {

                    if (i == 0)
                    {
                      char spacer[4];
                      sprintf(spacer, "\n");
                      waddstr(win1, spacer);
                      
                      char selectind[4];
                      

                      char clastlap[26];
                      sprintf(clastlap, "  session name  ");
                      waddstr(win1, clastlap);

                      wprintw(win1, "   ");

                      char cbestlap[14];
                      sprintf(cbestlap, " laps ");
                      waddstr(win1, cbestlap);

                      wprintw(win1, "\n");
                    
                    }
                    else
                    {
                      
                      char spacer[4];
                      sprintf(spacer, "\n");
                      waddstr(win1, spacer);
                      
                      char selectind[6];
                      
                      if ( i == selection )
                      {
                        useid = sess.rows[i-1].session_id;
                        sprintf(selectind, "_*_");
                        waddstr(win1, selectind);

                      }
                      else
                      {
                        sprintf(selectind, "___");
                        waddstr(win1, selectind);
                      }
                      int maxstrlen = 20;
                      wprintw(win1, " %i  ", sess.rows[i-1].session_id);

                      char clastlap[14];
                      sprintf(clastlap, "session name");
                      waddstr(win1, clastlap);

                      wprintw(win1, "   ");
                      char cbestlap[14];
                      sprintf(cbestlap, "laps %i", sess.rows[i-1].laps);
                      waddstr(win1, cbestlap);

                      wprintw(win1, "\n");
                    }
                  //box(win1, 0, 0);
                  //wrefresh(win1);
              }
              break;
            case STINTS_SCREEN:
              if (stintsid == 0)
              {
                break;
              }
              for(int i=0; i<stintsid+1; i++)
              {
                    if (i == 0)
                    {
                      char spacer[4];
                      sprintf(spacer, "\n");
                      waddstr(win1, spacer);
                      
                      char selectind[4];
                      

                      char clastlap[26];
                      sprintf(clastlap, "  stint name  ");
                      waddstr(win1, clastlap);

                      wprintw(win1, "   ");

                      char cbestlap[14];
                      sprintf(cbestlap, " laps ");
                      waddstr(win1, cbestlap);

                      wprintw(win1, "\n");
                    
                    }
                    else
                    {
                      
                      char spacer[4];
                      sprintf(spacer, "\n");
                      waddstr(win1, spacer);
                      
                      char selectind[4];
                      
                      if ( i == selection )
                      {
                        sprintf(selectind, "_*_");
                        waddstr(win1, selectind);
                        useid = stints.rows[i-i].stint_id;
                      }
                      else
                      {
                        sprintf(selectind, "___");
                        waddstr(win1, selectind);
                      }
                      int maxstrlen = 20;
                      wprintw(win1, " %i  ", stints.rows[i-1].stint_id);

                      char clastlap[14];
                      sprintf(clastlap, "stint name");
                      waddstr(win1, clastlap);

                      wprintw(win1, "   ");

                      char cbestlap[14];
                      sprintf(cbestlap, "laps %i", stints.rows[i-1].driver_id);
                      waddstr(win1, cbestlap);

                      wprintw(win1, "\n");
                    }
              }   
              break;
              //wattrset(win4, COLOR_PAIR(1));
            case LAPS_SCREEN:
              if (lapsresults == 0)
              {
                break;
              }
              for(int i=0; i<lapsresults+1; i++)
              {
                    if (i == 0)
                    {
                      char spacer[4];
                      sprintf(spacer, "\n");
                      waddstr(win1, spacer);

                      char selectind[4];


                      char clastlap[16];
                      sprintf(clastlap, "  lap name  ");
                      waddstr(win1, clastlap);

                      wprintw(win1, "   ");

                      char cbestlap[14];
                      sprintf(cbestlap, " tyre ");
                      waddstr(win1, cbestlap);

                      wprintw(win1, "\n");

                    }
                    else
                    {

                      char spacer[4];
                      sprintf(spacer, "\n");
                      waddstr(win1, spacer);

                      char selectind[4];

                      if ( i == selection )
                      {
                        sprintf(selectind, "_*_");
                        waddstr(win1, selectind);
                      }
                      else
                      {
                        sprintf(selectind, "___");
                        waddstr(win1, selectind);
                      }

                      char selectind1[4];
                      if ( i == selection1 )
                      {
                        sprintf(selectind1, "_1_");
                        waddstr(win1, selectind1);
                      }
                      else if ( i == selection2 )
                      {
                        sprintf(selectind1, "_2_");
                        waddstr(win1, selectind1);
                      }
                      else
                      {
                        sprintf(selectind1, "___");
                        waddstr(win1, selectind1);
                      }

                      int maxstrlen = 20;
                      wprintw(win1, " %i  ", lapsdb.rows[i-1].lap_id);

                      char clastlap[14];
                      sprintf(clastlap, "lap name");
                      waddstr(win1, clastlap);

                      wprintw(win1, "   ");

                      char cbestlap[14];
                      sprintf(cbestlap, "laps %i", lapsdb.rows[i-1].max_speed);
                      waddstr(win1, cbestlap);

                      wprintw(win1, "   ");
                      char gripstr[14];
                      sprintf(gripstr, "%s", lapsdb.rows[i-1].tyre);
                      waddstr(win1, gripstr);
                      
                      wprintw(win1, "   ");
                      char finstr[30];
                      sprintf(finstr, " %s ", lapsdb.rows[i-1].finished_at);
                      waddstr(win1, finstr);


                      wprintw(win1, "\n");
                    }
              }
              break;
              //wattrset(win4, COLOR_PAIR(1));
          }
            action = 0;
        }
        box(win1, 0, 0);
        //box(win2, 0, 0);
        //box(win3, 0, 0);
        //box(win4, 0, 0);

        wrefresh(win1);
        //wrefresh(win2);
        //wrefresh(win3);
        //wrefresh(win4);

        scanf("%c", &ch);
        if(ch == 'q')
        {
            go = false;
        }
        if (ch == 'b')
        {
            switch(screen) {

              case STINTS_SCREEN:
                action = 2;
                screen = SESSIONS_SCREEN;


                break;
              case LAPS_SCREEN:
                action = 3;
                screen = STINTS_SCREEN;
                break;
            }
                selection = 1;
                lastselection = 1;
        }
        if (ch == 'e')
        {
            switch(screen) {

              case SESSIONS_SCREEN:
                action = 3;
                screen = STINTS_SCREEN;
                

                break;
              case STINTS_SCREEN:
                action = 4;
                screen = LAPS_SCREEN;
                break;
            }

                selection = 1;
                lastselection = 1;
        }
        if (ch == 'B')
        {
            selection++;
            if (selection > curresults)
            {
              selection = curresults;
            }
        }
        if (ch == 'A')
        {
            selection--;
            if (selection <= 1)
            {
              selection = 1;
            }
        }
        if (ch == '1')
        {
          selection1 = selection;
          action = 1;
        }
        if (ch == '2')
        {
          selection2 = selection;
          action = 1;
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

    h_close_db(conn);
    h_clean_connection(conn);

    //return 0;
}

