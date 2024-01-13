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

#include <byteswap.h>

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


WINDOW* bwin1;
WINDOW* bwin2;
WINDOW* bwin3;
WINDOW* bwin4;

int bwinx, bwiny;

char blanks[100];

void add_line()
{
    wprintw(bwin1, "\n");
    for (int j = 0;  j < bwiny-1;  ++j)
    {
        waddch(bwin1, ACS_HLINE);
    }
}

void set_spaces(int spaces)
{
    blanks[spaces] = '\0';
}

void b_handle_winch(int sig)
{
    endwin();

    refresh();
    clear();
    getmaxyx(stdscr, bwinx, bwiny);
    //win23y = winy/3;
    //win23x = winx/3;
    //win1 = newwin(winx,winy,0,0);
    //win2 = newwin(win23x,win23y,1,win23y-1);
    //win3 = newwin(win23x,win23y,1,win23y*2-1);
    //win4 = newwin(winx-win23x-2,winy-win23y,win23x+1,win23y-1);
    refresh();
}

//void rectangle(int y1, int x1, int y2, int x2)
//{
//    mvhline(y1, x1, 0, x2-x1);
//    mvhline(y2, x1, 0, x2-x1);
//    mvvline(y1, x1, 0, y2-y1);
//    mvvline(y1, x2, 0, y2-y1);
//    mvaddch(y1, x1, ACS_ULCORNER);
//    mvaddch(y2, x1, ACS_LLCORNER);
//    mvaddch(y1, x2, ACS_URCORNER);
//    mvaddch(y2, x2, ACS_LRCORNER);
//}

int b_curses_init()
{
    memset(blanks,' ',sizeof(blanks));
    initscr();
    start_color();

    init_pair(1,COLOR_GREEN,0);
    init_pair(2,COLOR_YELLOW,0);
    init_pair(3,COLOR_MAGENTA,0);
    init_pair(4,COLOR_WHITE,0);

    getmaxyx(stdscr, bwinx, bwiny);
    slogt("windowx %i, windowy %i", bwinx, bwiny);
    bwin1 = newwin(bwinx,bwiny,0,0);
    //win23y = winy/3;
    //win23x = winx/3;
    //win2 = newwin(win23x,win23y,1,win23y-1);
    //win3 = newwin(win23x,win23y,1,win23y*2-1);
    //win4 = newwin(winx-win23x-2,winy-win23y,win23x+1,win23y-1);

    wbkgd(bwin1,COLOR_PAIR(1));
    wbkgd(bwin2,COLOR_PAIR(1));
    wbkgd(bwin3,COLOR_PAIR(1));
    wbkgd(bwin4,COLOR_PAIR(1));

    signal(SIGWINCH, b_handle_winch);
    cbreak();
    noecho();

    box(bwin1, 0, 0);
    box(bwin2, 0, 0);
    box(bwin3, 0, 0);
    box(bwin4, 0, 0);
}


int telem_result(struct _h_result result, int doublefields, int intfields, int* intarrays, double* doublearrays) {
  int col, row, i;
  char buf[64];
  slogt("rows: %d, col: %d", result.nb_rows, result.nb_columns);
  //int* intarrays;
  int points = 0;
  //int doublefields = 3;
  //int intfields = 3;
  //int* intarrays;
  //int* doublearrays;
  //int* intarrays = malloc((sizeof(int)*1736)*3);
  //double* doublearrays = malloc((sizeof(double)*1736)*3);
  int intarrayoffset = 0;
  int doublearrayoffset = 0;

  for (row = 0; row<result.nb_rows; row++) {
    for (col=0; col<result.nb_columns; col++) {
      switch(result.data[row][col].type) {
        case HOEL_COL_TYPE_INT:
          int cc = ((struct _h_type_int *)result.data[row][col].t_data)->value;
          if (col == 1)
          {
            points = cc;
          }
          break;
        case HOEL_COL_TYPE_DOUBLE:
          //intarrays = malloc((sizeof(int)*1736)*3);
          break;
        case HOEL_COL_TYPE_TEXT:
          slogi("| %s ", ((struct _h_type_text *)result.data[row][col].t_data)->value);
          break;
        case HOEL_COL_TYPE_BLOB:
          int offset2 = 0;

          int j = 0;
          i = 2;
          if (col < 5)
          {
          while (i<((struct _h_type_blob *)result.data[row][col].t_data)->length)
          {
            char sss[10];
            sss[0] = '0';
            sss[1] = 'x';
            sss[2] = *((char*)((struct _h_type_blob *)result.data[row][col].t_data)->value+offset2+2);
            sss[3] = *((char*)((struct _h_type_blob *)result.data[row][col].t_data)->value+offset2+3);
            sss[4] = *((char*)((struct _h_type_blob *)result.data[row][col].t_data)->value+offset2+4);
            sss[5] = *((char*)((struct _h_type_blob *)result.data[row][col].t_data)->value+offset2+5);
            sss[6] = *((char*)((struct _h_type_blob *)result.data[row][col].t_data)->value+offset2+6);
            sss[7] = *((char*)((struct _h_type_blob *)result.data[row][col].t_data)->value+offset2+7);
            sss[8] = *((char*)((struct _h_type_blob *)result.data[row][col].t_data)->value+offset2+8);
            sss[9] = *((char*)((struct _h_type_blob *)result.data[row][col].t_data)->value+offset2+9);
            long val;
            int number = (int)strtol(sss, NULL, 16);
            int swapped = __bswap_32(number);

            intarrays[j+intarrayoffset] = __bswap_32(number);
            offset2 = offset2 + 8;
            i+=8;
            j++;
          }
          intarrayoffset += points;
          }
          else
          {
          while (i<((struct _h_type_blob *)result.data[row][col].t_data)->length)
          {
            char sss[18];
            sss[0] = '0';
            sss[1] = 'x';
            sss[2] = *((char*)((struct _h_type_blob *)result.data[row][col].t_data)->value+offset2+2);
            sss[3] = *((char*)((struct _h_type_blob *)result.data[row][col].t_data)->value+offset2+3);
            sss[4] = *((char*)((struct _h_type_blob *)result.data[row][col].t_data)->value+offset2+4);
            sss[5] = *((char*)((struct _h_type_blob *)result.data[row][col].t_data)->value+offset2+5);
            sss[6] = *((char*)((struct _h_type_blob *)result.data[row][col].t_data)->value+offset2+6);
            sss[7] = *((char*)((struct _h_type_blob *)result.data[row][col].t_data)->value+offset2+7);
            sss[8] = *((char*)((struct _h_type_blob *)result.data[row][col].t_data)->value+offset2+8);
            sss[9] = *((char*)((struct _h_type_blob *)result.data[row][col].t_data)->value+offset2+9);
            sss[2+8] = *((char*)((struct _h_type_blob *)result.data[row][col].t_data)->value+offset2+2+8);
            sss[3+8] = *((char*)((struct _h_type_blob *)result.data[row][col].t_data)->value+offset2+3+8);
            sss[4+8] = *((char*)((struct _h_type_blob *)result.data[row][col].t_data)->value+offset2+4+8);
            sss[5+8] = *((char*)((struct _h_type_blob *)result.data[row][col].t_data)->value+offset2+5+8);
            sss[6+8] = *((char*)((struct _h_type_blob *)result.data[row][col].t_data)->value+offset2+6+8);
            sss[7+8] = *((char*)((struct _h_type_blob *)result.data[row][col].t_data)->value+offset2+7+8);
            sss[8+8] = *((char*)((struct _h_type_blob *)result.data[row][col].t_data)->value+offset2+8+8);
            sss[9+8] = *((char*)((struct _h_type_blob *)result.data[row][col].t_data)->value+offset2+9+8);
            long val;
            int64_t number = (int64_t) strtoll(sss, NULL, 16);
            int64_t swapped = __bswap_64(number);
            double d = *((double*)&swapped);
            doublearrays[j+doublearrayoffset] = (double) d;
            offset2 = offset2 + 16;
            i+=16;
            j++;
          }
          doublearrayoffset += points;

          }
          //snprintf( "blob value: %.*s", ((struct _h_type_blob *)result.data[row][col].t_data)->length, ((struct _h_type_blob *)result.data[row][col].t_data)->value);
          //char* b = malloc(sizeof(int)*1736);
          //for (i=0; i<((struct _h_type_blob *)result.data[row][col].t_data)->length; i++) {
          //  //slogi("%c", *((char*)(((struct _h_type_blob *)result.data[row][col].t_data)->value+i)));
          //  memcpy(&b[i], ((struct _h_type_blob *)result.data[row][col].t_data)->value+1*sizeof(char), sizeof(char));
          //}
//          FILE *out = fopen("memory.bin", "wb");
//  if(out != NULL)
//  {
//    size_t to_go = sizeof(int)*1736;
//    while(to_go > 0)
//    {
//      const size_t wrote = fwrite(b, to_go, 1, out);
//      if(wrote == 0)
//        break;
//      to_go -= wrote;
//    }
//    fclose(out);
//  }
          break;
        case HOEL_COL_TYPE_DATE:
          strftime(buf, 64, "%Y-%m-%d %H:%M:%S", &((struct _h_type_datetime *)result.data[row][col].t_data)->value);
          printf("| %s ", buf);
        case HOEL_COL_TYPE_NULL:
          slogi("| [null] ");
          break;
      }
    }
    printf("|\n");
  }

  return points;
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


int getsessions(struct _h_connection* conn, int* err, const char* sessionname, SessionDbo* sess)
{
    struct _h_result result;
    struct _h_data * data;
    char* query = malloc(99 * sizeof(char));
    slogt("Performing query");

    //sprintf(query, "select session_id, event_id, event_type, duration_min, elapsed_ms, laps, air_temp, road_temp, start_grip, current_grip, is_finished, http_port from %s", "Sessions");
    sprintf(query, "select session_id, event_id, event_type, laps FROM %s ORDER BY session_id DESC LIMIT 25", "Sessions");
    if (h_query_select(conn, query, &result) == H_OK) {
        sess->rows = malloc(sizeof(SessionRowData) * result.nb_rows);
        get_row_results(result, sess->fields, sess->rows, sizeof(SessionRowData));
        //get_session_result(result, sess);
        h_clean_result(&result);
    }
    else
    {
        printf("Error executing query\n");
        *err = E_DB_QUERY_FAIL;;
        free(query);
        return 0;
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
    slogt("execute query %s", query);
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


    sprintf(query, "select * FROM %s WHERE %s=%i", "Laps", "stint_id", use_id);
    if (h_query_select(conn, query, &result) == H_OK) {
        laps->rows = malloc(sizeof(LapRowData) * result.nb_rows);
        get_row_results(result, laps->fields, laps->rows, sizeof(LapRowData));
        //print_result(result);
        //get_stint_result(result, stint);
        h_clean_result(&result);
    } else {
        printf("Error executing query\n");
    }
    free(query);

    return result.nb_rows;
}

int dumptelemetrytofile(struct _h_connection* conn, char* datadir, int lap1id, int lap2id)
{

    slogt("dumping telemetry to temp file");

    int points = 0;
    int intfields = 3;
    int doublefields = 3;

    struct _h_result result;
    struct _h_data *data;
    char* query = malloc(150 * sizeof(char));
    sprintf(query, "SELECT lap_id, points FROM %s WHERE %s=%i", "telemetry", "lap_id", lap1id);
    if (h_query_select(conn, query, &result) == H_OK) {
        //laps->rows = malloc(sizeof(LapRowData) * result.nb_rows);
        //get_row_results(result, laps->fields, laps->rows, sizeof(LapRowData));
        points = telem_result(result, 3, 3, NULL, NULL);
        //get_stint_result(result, stint);
        h_clean_result(&result);
    } else {
        printf("Error executing query\n");
    }
    free(query);


    int* intarrays1 = malloc((sizeof(int))*points*intfields);
    double* doublearrays1 = malloc((sizeof(double))*points*doublefields);
    int* intarrays2 = malloc((sizeof(int))*points*intfields);
    double* doublearrays2 = malloc((sizeof(double))*points*doublefields);

    struct _h_result result1;
    struct _h_data * data1;
    char* query1 = malloc(150 * sizeof(char));
    sprintf(query1, "SELECT lap_id, points, speed, gear, rpms, brake, accel, steer FROM %s WHERE %s=%i", "telemetry", "lap_id", lap1id);
    if (h_query_select(conn, query1, &result1) == H_OK) {
        //laps->rows = malloc(sizeof(LapRowData) * result.nb_rows);
        //get_row_results(result, laps->fields, laps->rows, sizeof(LapRowData));
        points = telem_result(result1, intfields, doublefields, intarrays1, doublearrays1);
        //get_stint_result(result, stint);
        h_clean_result(&result1);
    } else {
        printf("Error executing query\n");
    }
    free(query1);

    struct _h_result result2;
    struct _h_data * data2;
    char* query2 = malloc(150 * sizeof(char));
    sprintf(query2, "SELECT lap_id, points, speed, gear, rpms, brake, accel, steer FROM %s WHERE %s=%i", "telemetry", "lap_id", lap2id);
    if (h_query_select(conn, query2, &result2) == H_OK) {
        //laps->rows = malloc(sizeof(LapRowData) * result.nb_rows);
        //get_row_results(result, laps->fields, laps->rows, sizeof(LapRowData));
        points = telem_result(result2, intfields, doublefields, intarrays2, doublearrays2);
        //get_stint_result(result, stint);
        h_clean_result(&result2);
    } else {
        printf("Error executing query\n");
    }
    free(query2);

    char* filename1= "data.out";
    size_t strsize = strlen(datadir) + strlen(filename1) + 1;
    char* datafile = malloc(strsize);

    snprintf(datafile, strsize, "%s%s", datadir, filename1);
    slogt("dumping %i points to file %s", points, datafile);
    FILE *out = fopen(datafile, "w");
    fprintf(out, "%s %s %s %s %s %s %s %s %s %s %s %s %s\n", "point", "speed1", "gear1", "rpms1", "brake1", "accel1", "steer1", "speed2", "gear2", "rpms2", "brake2", "accel2", "steer2" );
    for (int i=0; i<points; i++)
    {
      fprintf(out, "%i %i %i %i %f %f %f", i+1, intarrays1[i], intarrays1[i+points], intarrays1[i+(points*2)], doublearrays1[i], doublearrays1[i+points], doublearrays1[i+(points*2)]);
      fprintf(out, "%i %i %i %i %f %f %f\n", i+1, intarrays2[i], intarrays2[i+points], intarrays2[i+(points*2)], doublearrays2[i], doublearrays2[i+points], doublearrays2[i+(points*2)]);
    }
    fclose(out);

    free(intarrays1);
    free(intarrays2);
    free(doublearrays1);
    free(doublearrays2);

    return 1;
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





void* browseloop(Parameters* p, char* datadir)
{

    struct _h_result result;
    struct _h_connection * conn;
    conn = h_connect_pgsql(p->db_conn);

    if (conn == NULL)
    {
      slogf("Unable to connect to configured Gilles database. Are the parameters in the config correct? Is the user allowed to access from this address?");
      p->err = E_FAILED_DB_CONN;
      return 0;
    }

    slogt("Starting analyzer");
    b_curses_init();

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
    DBField f_tyre_temp;
    f_tyre_temp.type = HOEL_COL_TYPE_DOUBLE;
    f_tyre_temp.offset = offsetof(LapRowData, f_tyre_temp);
    DBField r_tyre_temp;
    r_tyre_temp.type = HOEL_COL_TYPE_DOUBLE;
    r_tyre_temp.offset = offsetof(LapRowData, r_tyre_temp);
    DBField f_tyre_wear;
    f_tyre_wear.type = HOEL_COL_TYPE_DOUBLE;
    f_tyre_wear.offset = offsetof(LapRowData, f_tyre_wear);
    DBField r_tyre_wear;
    r_tyre_wear.type = HOEL_COL_TYPE_DOUBLE;
    r_tyre_wear.offset = offsetof(LapRowData, r_tyre_wear);
    DBField f_tyre_press;
    f_tyre_press.type = HOEL_COL_TYPE_DOUBLE;
    f_tyre_press.offset = offsetof(LapRowData, f_tyre_press);
    DBField r_tyre_press;
    r_tyre_press.type = HOEL_COL_TYPE_DOUBLE;
    r_tyre_press.offset = offsetof(LapRowData, r_tyre_press);
    DBField f_brake_temp;
    f_brake_temp.type = HOEL_COL_TYPE_DOUBLE;
    f_brake_temp.offset = offsetof(LapRowData, f_brake_temp);
    DBField r_brake_temp;
    r_brake_temp.type = HOEL_COL_TYPE_DOUBLE;
    r_brake_temp.offset = offsetof(LapRowData, r_brake_temp);
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
    lapsdb.fields[13] = f_tyre_temp;
    lapsdb.fields[14] = r_tyre_temp;
    lapsdb.fields[15] = f_tyre_wear;
    lapsdb.fields[16] = r_tyre_wear;
    lapsdb.fields[17] = f_tyre_press;
    lapsdb.fields[18] = r_tyre_press;
    lapsdb.fields[19] = f_brake_temp;
    lapsdb.fields[20] = r_brake_temp;

    //slogt("sessions has %i rows", sess.numrows);

    action = 2;


    int go = true;
    char lastsimstatus = false;

    int screen = SESSIONS_SCREEN;
    char ch;
    box(bwin1, 0, 0);
    wrefresh(bwin1);
    int stint_useid = 0;
    int lap_useid = 0;
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
                int err = 0;
                sessions = getsessions(conn, &err, "Sessions", &sess);
                if (err != E_NO_ERROR)
                {
                  go = false;
                }
                else
                {
                  curresults = sessions;
                }
        }

        if (action == 3)
        {
                slogt("going to perform an action");

                stintsid = getstints(conn, "Stints", &stints, stint_useid);
                curresults = stintsid;
        }
        if (action == 4)
        {
                slogt("going to perform an action");

                lapsresults = getlaps(conn, "laps", &lapsdb, lap_useid);
                curresults = lapsresults;
        }

        if (action > 0)
        {
            wclear(bwin1);

            wprintw(bwin1, "\n");
            switch(screen) {
            case SESSIONS_SCREEN:
              for(int i=0; i<sessions+1; i++)
              {

                    if (i == 0)
                    {
                      wattrset(bwin1, COLOR_PAIR(2));
                      wattron(bwin1, A_BOLD);
                      set_spaces(bwiny/2);
                      waddstr(bwin1, blanks);

                      char sessions_title[9];
                      sprintf(sessions_title, "Sessions");
                      waddstr(bwin1, sessions_title);
                      wprintw(bwin1, "\n");




                      wprintw(bwin1, "\n");
                      
                      char selectind[4];
                      
                      set_spaces(bwiny/4);
                      waddstr(bwin1, blanks);

                      char idx[6];
                      sprintf(idx, "   idx  ");
                      waddstr(bwin1, idx);

                      set_spaces(bwiny/6);
                      waddstr(bwin1, blanks);

                      char clastlap[26];
                      sprintf(clastlap, "  session name  ");
                      waddstr(bwin1, clastlap);

                      set_spaces(bwiny/6);
                      waddstr(bwin1, blanks);

                      char cbestlap[14];
                      sprintf(cbestlap, " laps ");
                      waddstr(bwin1, cbestlap);

                      add_line();

                      //mvhline(5, 5, 0, bwiny-1);
                      //mvwhline(bwin1, 20, 20, ACS_HLINE, bwiny+1);
                      //whline(bwin1, ACS_HLINE, 100);

                      wprintw(bwin1, "\n");
                    
                      wattroff(bwin1, A_BOLD);
                      wattrset(bwin1, COLOR_PAIR(1));
                    }
                    else
                    {
                      
                      char spacer[4];
                      sprintf(spacer, "\n");
                      waddstr(bwin1, spacer);
                      
                      char selectind[6];
                      
                      if ( i == selection )
                      {
                        stint_useid = sess.rows[i-1].session_id;
                        sprintf(selectind, "_*_");
                        waddstr(bwin1, selectind);

                      }
                      else
                      {
                        sprintf(selectind, "___");
                        waddstr(bwin1, selectind);
                      }
                      int maxstrlen = 20;
                      wprintw(bwin1, " %i  ", sess.rows[i-1].session_id);

                      char clastlap[14];
                      sprintf(clastlap, "session name");
                      waddstr(bwin1, clastlap);

                      wprintw(bwin1, "   ");
                      char cbestlap[14];
                      sprintf(cbestlap, "laps %i", sess.rows[i-1].laps);
                      waddstr(bwin1, cbestlap);

                      wprintw(bwin1, "\n");
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
                      waddstr(bwin1, spacer);
                      
                      char selectind[4];
                      

                      char clastlap[26];
                      sprintf(clastlap, "  stint name  ");
                      waddstr(bwin1, clastlap);

                      wprintw(bwin1, "   ");

                      char cbestlap[14];
                      sprintf(cbestlap, " laps ");
                      waddstr(bwin1, cbestlap);

                      wprintw(bwin1, "\n");
                    
                    }
                    else
                    {
                      
                      char spacer[4];
                      sprintf(spacer, "\n");
                      waddstr(bwin1, spacer);
                      
                      char selectind[4];
                      
                      if ( i == selection )
                      {
                        sprintf(selectind, "_*_");
                        waddstr(bwin1, selectind);
                        lap_useid = stints.rows[i-i].stint_id;
                      }
                      else
                      {
                        sprintf(selectind, "___");
                        waddstr(bwin1, selectind);
                      }
                      int maxstrlen = 20;
                      wprintw(bwin1, " %i  ", stints.rows[i-1].stint_id);

                      char clastlap[14];
                      sprintf(clastlap, "stint name");
                      waddstr(bwin1, clastlap);

                      wprintw(bwin1, "   ");

                      char cbestlap[14];
                      sprintf(cbestlap, "laps %i", stints.rows[i-1].laps);
                      waddstr(bwin1, cbestlap);

                      wprintw(bwin1, "\n");
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
                      waddstr(bwin1, spacer);

                      char selectind[4];


                      char clastlap[16];
                      sprintf(clastlap, "  lap name  ");
                      waddstr(bwin1, clastlap);

                      wprintw(bwin1, "   ");

                      char cbestlap[14];
                      sprintf(cbestlap, " tyre ");
                      waddstr(bwin1, cbestlap);

                      wprintw(bwin1, "\n");

                    }
                    else
                    {

                      char spacer[4];
                      sprintf(spacer, "\n");
                      waddstr(bwin1, spacer);

                      char selectind[4];

                      if ( i == selection )
                      {
                        sprintf(selectind, "_*_");
                        waddstr(bwin1, selectind);
                      }
                      else
                      {
                        sprintf(selectind, "___");
                        waddstr(bwin1, selectind);
                      }

                      char selectind1[4];
                      if ( lapsdb.rows[i-1].lap_id == selection1 )
                      {
                        sprintf(selectind1, "_1_");
                        waddstr(bwin1, selectind1);
                      }
                      else if ( lapsdb.rows[i-1].lap_id == selection2 )
                      {
                        sprintf(selectind1, "_2_");
                        waddstr(bwin1, selectind1);
                      }
                      else
                      {
                        sprintf(selectind1, "___");
                        waddstr(bwin1, selectind1);
                      }

                      int maxstrlen = 20;
                      wprintw(bwin1, " %i  ", lapsdb.rows[i-1].lap_id);

                      char clastlap[14];
                      sprintf(clastlap, "lap name");
                      waddstr(bwin1, clastlap);

                      wprintw(bwin1, "   ");

                      char cbestlap[14];
                      sprintf(cbestlap, "laps %i", lapsdb.rows[i-1].max_speed);
                      waddstr(bwin1, cbestlap);

                      wprintw(bwin1, "   ");
                      char gripstr[14];
                      sprintf(gripstr, "%s", lapsdb.rows[i-1].tyre);
                      waddstr(bwin1, gripstr);
                      
                      wprintw(bwin1, "   ");
                      char finstr[29];
                      sprintf(finstr, " %s ", lapsdb.rows[i-1].finished_at);
                      waddstr(bwin1, finstr);


                      wprintw(bwin1, "\n");
                    }
              }
              break;
              //wattrset(win4, COLOR_PAIR(1));
          }
            action = 0;
        }
        box(bwin1, 0, 0);
        //box(win2, 0, 0);
        //box(win3, 0, 0);
        //box(win4, 0, 0);

        wrefresh(bwin1);
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
        if (ch == 'g')
        {
            selection1 = 363;
            selection2 = 362;
            if (selection1 > 0 && selection2 > 0)
            {
              dumptelemetrytofile(conn, datadir, selection1, selection2);

              slogt("finished dumping data");
              size_t strsize = strlen(datadir) + strlen(p->gnuplot_file) + 1;
              char* plotfile = malloc(strsize);
              snprintf(plotfile, strsize, "%s%s", datadir, p->gnuplot_file);
              static char* argv1[]={"gnuplot", "-p", "plotfile.gp", NULL};
              argv1[2] = plotfile;
              slogi("Using gnu plot file %s", plotfile);
              if(!fork())
              {
                  execv("/usr/bin/gnuplot", argv1);
              }
              //wait(NULL);
            }
            action = 4;
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
          selection1 = lapsdb.rows[selection-1].lap_id;
          action = 1;
        }
        if (ch == '2')
        {
          selection2 = lapsdb.rows[selection-1].lap_id;
          action = 1;
        }

        
    }


    wrefresh(bwin4);
    delwin(bwin4);
    endwin();

    wrefresh(bwin3);
    delwin(bwin3);
    endwin();

    wrefresh(bwin2);
    delwin(bwin2);
    endwin();

    wrefresh(bwin1);
    delwin(bwin1);
    endwin();

    h_close_db(conn);
    h_clean_connection(conn);

    //return 0;
}

