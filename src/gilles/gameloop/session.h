
#ifndef _SESSION_H
#define _SESSION_H

#include <stddef.h>
#include <stdbool.h>

typedef struct DBData DBData;
typedef struct DBField DBField;
typedef struct SessionFieldsData SessionFieldsData;
typedef struct SessionFields SessionFields;
typedef struct Session Session;
typedef unsigned char PDBTimeStamp[26];

struct DBField
{
    size_t offset;
    size_t size;
    int colnum;
    int type;
    char name[20];
};

//struct SessionFields
//{
//    DBField session_id;
//    DBField laps;
//};

typedef struct SessionRowData
{
    int session_id;
    int event_id;
    int event_type;
    //char track_time[5];
    //char session_name[50];
    //char start_time[6];
    //int duration_min;
    //int elapsed_ms;
    int laps;
    //char weather[100];
    //double air_temp;
    //double road_temp;
    //double start_grip;
    //double current_grip;
    //int is_finished;
    //char finish_time[6];
    //char last_activity[6];
    //int http_port;

}
SessionRowData;

//struct SessionFieldsData
//{
//    int session_id;
//    int laps;
//};

typedef struct StintRowData
{
    int stint_id;
    int driver_id;
    int team_member_id;
    int session_id;
    int car_id;
    int game_car_id;
    int laps;
    int valid_laps;
    int best_lap_id;
}
StintRowData;

typedef struct LapRowData
{
    int lap_id;
    int stint_id;
    int sector_1;
    int sector_2;
    int sector_3;
    double grip;
    unsigned char tyre[10];
    int time;
    int cuts;
    int crashes;
    int max_speed;
    int avg_speed;
    double f_tyre_temp;
    double r_tyre_temp;
    double f_tyre_wear;
    double r_tyre_wear;
    double f_tyre_press;
    double r_tyre_press;
    double f_brake_temp;
    double r_brake_temp;
    PDBTimeStamp finished_at;
}
LapRowData;

typedef struct SessionDbo
{
    int numrows;
    bool hasdata;

    DBField fields[4];
    SessionRowData* rows;
}
SessionDbo;

typedef struct StintDbo
{
    int numrows;
    bool hasdata;

    DBField fields[9];
    StintRowData* rows;
}
StintDbo;

typedef struct LapDbo
{
    int numrows;
    bool hasdata;

    DBField fields[21];
    LapRowData* rows;
}
LapDbo;

#endif
