
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <hoel.h>
#include <jansson.h>

#include "telemetry.h"
#include "../helper/confighelper.h"
#include "../slog/slog.h"

int telem_result(struct _h_result result, int doublefields, int intfields, int* intarrays, double* doublearrays)
{
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

    for (row = 0; row<result.nb_rows; row++)
    {
        for (col=0; col<result.nb_columns; col++)
        {
            switch(result.data[row][col].type)
            {
                case HOEL_COL_TYPE_INT:
                    int cc = ((struct _h_type_int*)result.data[row][col].t_data)->value;
                    if (col == 1)
                    {
                        points = cc;
                    }
                    break;
                case HOEL_COL_TYPE_DOUBLE:
                    //intarrays = malloc((sizeof(int)*1736)*3);
                    break;
                case HOEL_COL_TYPE_TEXT:
                    slogi("| %s ", ((struct _h_type_text*)result.data[row][col].t_data)->value);
                    break;
                case HOEL_COL_TYPE_BLOB:
                    int offset2 = 0;

                    int j = 0;
                    i = 2;
                    if (col < 5)
                    {
                        while (i<((struct _h_type_blob*)result.data[row][col].t_data)->length)
                        {
                            char sss[10];
                            sss[0] = '0';
                            sss[1] = 'x';
                            sss[2] = *((char*)((struct _h_type_blob*)result.data[row][col].t_data)->value+offset2+2);
                            sss[3] = *((char*)((struct _h_type_blob*)result.data[row][col].t_data)->value+offset2+3);
                            sss[4] = *((char*)((struct _h_type_blob*)result.data[row][col].t_data)->value+offset2+4);
                            sss[5] = *((char*)((struct _h_type_blob*)result.data[row][col].t_data)->value+offset2+5);
                            sss[6] = *((char*)((struct _h_type_blob*)result.data[row][col].t_data)->value+offset2+6);
                            sss[7] = *((char*)((struct _h_type_blob*)result.data[row][col].t_data)->value+offset2+7);
                            sss[8] = *((char*)((struct _h_type_blob*)result.data[row][col].t_data)->value+offset2+8);
                            sss[9] = *((char*)((struct _h_type_blob*)result.data[row][col].t_data)->value+offset2+9);
                            long val;
                            uint32_t number = (uint32_t)strtol(sss, NULL, 16);
                            uint32_t swapped = (uint32_t)__bswap_32(number);

                            intarrays[j+intarrayoffset] = (uint32_t)__bswap_32(number);
                            offset2 = offset2 + 8;
                            i+=8;
                            j++;
                        }
                        intarrayoffset += points;
                    }
                    else
                    {
                        while (i<((struct _h_type_blob*)result.data[row][col].t_data)->length)
                        {
                            char sss[18];
                            sss[0] = '0';
                            sss[1] = 'x';
                            sss[2] = *((char*)((struct _h_type_blob*)result.data[row][col].t_data)->value+offset2+2);
                            sss[3] = *((char*)((struct _h_type_blob*)result.data[row][col].t_data)->value+offset2+3);
                            sss[4] = *((char*)((struct _h_type_blob*)result.data[row][col].t_data)->value+offset2+4);
                            sss[5] = *((char*)((struct _h_type_blob*)result.data[row][col].t_data)->value+offset2+5);
                            sss[6] = *((char*)((struct _h_type_blob*)result.data[row][col].t_data)->value+offset2+6);
                            sss[7] = *((char*)((struct _h_type_blob*)result.data[row][col].t_data)->value+offset2+7);
                            sss[8] = *((char*)((struct _h_type_blob*)result.data[row][col].t_data)->value+offset2+8);
                            sss[9] = *((char*)((struct _h_type_blob*)result.data[row][col].t_data)->value+offset2+9);
                            sss[2+8] = *((char*)((struct _h_type_blob*)result.data[row][col].t_data)->value+offset2+2+8);
                            sss[3+8] = *((char*)((struct _h_type_blob*)result.data[row][col].t_data)->value+offset2+3+8);
                            sss[4+8] = *((char*)((struct _h_type_blob*)result.data[row][col].t_data)->value+offset2+4+8);
                            sss[5+8] = *((char*)((struct _h_type_blob*)result.data[row][col].t_data)->value+offset2+5+8);
                            sss[6+8] = *((char*)((struct _h_type_blob*)result.data[row][col].t_data)->value+offset2+6+8);
                            sss[7+8] = *((char*)((struct _h_type_blob*)result.data[row][col].t_data)->value+offset2+7+8);
                            sss[8+8] = *((char*)((struct _h_type_blob*)result.data[row][col].t_data)->value+offset2+8+8);
                            sss[9+8] = *((char*)((struct _h_type_blob*)result.data[row][col].t_data)->value+offset2+9+8);
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
                    strftime(buf, 64, "%Y-%m-%d %H:%M:%S", &((struct _h_type_datetime*)result.data[row][col].t_data)->value);
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

int dumptelemetrytofile(struct _h_connection* conn, char* datadir, int lap1id, int lap2id)
{

    slogt("dumping telemetry to temp file: lap1id: %d lap2id: %d", lap1id, lap2id);

    int points = 0;
    int intfields = 3;
    int doublefields = 3;

    struct _h_result result;
    struct _h_data* data;
    char* query = malloc(150 * sizeof(char));
    sprintf(query, "SELECT lap_id, points FROM %s WHERE %s=%i", "telemetry", "lap_id", lap1id);
    if (h_query_select(conn, query, &result) == H_OK)
    {
        //laps->rows = malloc(sizeof(LapRowData) * result.nb_rows);
        //get_row_results(result, laps->fields, laps->rows, sizeof(LapRowData));
        points = telem_result(result, 3, 3, NULL, NULL);
        //get_stint_result(result, stint);
        h_clean_result(&result);
    }
    else
    {
        printf("Error executing query\n");
    }
    free(query);


    uint32_t* intarrays1 = malloc((sizeof(uint32_t))*points*intfields);
    double* doublearrays1 = malloc((sizeof(double))*points*doublefields);
    uint32_t* intarrays2 = malloc((sizeof(uint32_t))*points*intfields);
    double* doublearrays2 = malloc((sizeof(double))*points*doublefields);

    struct _h_result result1;
    struct _h_data* data1;
    char* query1 = malloc(150 * sizeof(char));
    sprintf(query1, "SELECT lap_id, points, speed, rpms, gear, brake, accel, steer FROM %s WHERE %s=%i", "telemetry", "lap_id", lap1id);
    if (h_query_select(conn, query1, &result1) == H_OK)
    {
        //laps->rows = malloc(sizeof(LapRowData) * result.nb_rows);
        //get_row_results(result, laps->fields, laps->rows, sizeof(LapRowData));
        points = telem_result(result1, intfields, doublefields, intarrays1, doublearrays1);
        //get_stint_result(result, stint);
        h_clean_result(&result1);
    }
    else
    {
        printf("Error executing query\n");
    }
    free(query1);

    struct _h_result result2;
    struct _h_data* data2;
    char* query2 = malloc(150 * sizeof(char));
    sprintf(query2, "SELECT lap_id, points, speed, rpms, gear, brake, accel, steer FROM %s WHERE %s=%i", "telemetry", "lap_id", lap2id);
    if (h_query_select(conn, query2, &result2) == H_OK)
    {
        //laps->rows = malloc(sizeof(LapRowData) * result.nb_rows);
        //get_row_results(result, laps->fields, laps->rows, sizeof(LapRowData));
        points = telem_result(result2, intfields, doublefields, intarrays2, doublearrays2);
        //get_stint_result(result, stint);
        h_clean_result(&result2);
    }
    else
    {
        printf("Error executing query\n");
    }
    free(query2);

    char* filename1 = "data.out";
    size_t strsize = strlen(datadir) + strlen(filename1) + 1;
    char* datafile = malloc(strsize);

    snprintf(datafile, strsize, "%s%s", datadir, filename1);
    slogt("dumping %i points to file %s", points, datafile);
    FILE* out = fopen(datafile, "w");
    fprintf(out, "%s %s %s %s %s %s %s %s %s %s %s %s %s\n", "point", "speed1", "rpms1", "gear1", "brake1", "accel1", "steer1", "speed2", "rpms2", "gear2", "brake2", "accel2", "steer2" );

    // this could be made configurable at some point, but i think the graphs are too noisey with the shifts into neutral for each shift
    bool hideneutral = true;
    int lastgear1 = 0;
    int lastgear2 = 0;
    for (int i=0; i<points; i++)
    {
        int gear1 = intarrays1[i+(points*2)];
        int gear2 = intarrays2[i+(points*2)];
        double steer1 = doublearrays1[i+(points*2)];
        double steer2 = doublearrays2[i+(points*2)];
        if (steer1 == 0.00 || steer2 == 0.00)
        {
            continue;
        }
        if (hideneutral == true)
        {
            if( gear1 == 1)
            {
                gear1 = lastgear1;
            }
            if( gear2 == 1)
            {
                gear2 = lastgear2;
            }
        }
        fprintf(out, "%i %i %i %i %f %f %f", i+1, intarrays1[i], intarrays1[i+points], gear1, doublearrays1[i], doublearrays1[i+points], doublearrays1[i+(points*2)]);
        // make sure there is an extra space at the beginning of this
        fprintf(out, " %i %i %i %f %f %f\n", intarrays2[i], intarrays2[i+points], gear2, doublearrays2[i], doublearrays2[i+points], doublearrays2[i+(points*2)]);
        lastgear1 = gear1;
        lastgear2 = gear2;
    }
    fclose(out);

    free(intarrays1);
    free(intarrays2);
    free(doublearrays1);
    free(doublearrays2);
    free(datafile);

    return 1;
}

void print_bytes(void* ptr, int size)
{
    char* pp = malloc((size*2)+1);
    char output[(size * 2) + 1];
    char* ppp = &output[0];
    unsigned char* p = ptr;
    int i;
    for (i=0; i<size; i++)
    {
        slogt("%02hhX", p[i]);
        ppp += sprintf(ppp, "%02X", p[i]);
        //snprintf(pp, (size*2)+1, "%s%02hhX", pp, p[i]);
    }
    slogt("\n");
    slogt("bytes %s", output);
}

int updatetelemetry(struct _h_connection* conn, int telemid, int size, const char* column, void* data)
{

    //char *pp = malloc((size*2)+1);
    char output[(size * 2) + 1];
    char* ppp = &output[0];
    unsigned char* p = data;
    int i;
    for (i=0; i<size; i++)
    {
        ppp += sprintf(ppp, "%02hhX", p[i]);
        //snprintf(pp, (size*2)+1, "%s%02hhX", pp, p[i]);
    }

    char* query = malloc((sizeof(char)*71)+(sizeof(column))+(size*2)+1);
    sprintf(query, "UPDATE telemetry SET %s = decode('%s', 'hex') WHERE telemetry_id = %i", column, &output, telemid);
    int res1 = h_query_update(conn, query);
    //int res1 = h_insert(conn, j_query, NULL);
    slogt("got res %i", res1);
    free(query);

    return res1;
}

int updatetelemetrydata(struct _h_connection* conn, int tracksamples, int telemid, int lapid,
                        int* speeddata, int* rpmdata, int* geardata,
                        double* steerdata, double* acceldata, double* brakedata)
{
    int b = 0;
    b = updatetelemetry(conn, telemid, tracksamples*sizeof(double), "steer", steerdata);
    b = updatetelemetry(conn, telemid, tracksamples*sizeof(double), "accel", acceldata);
    b = updatetelemetry(conn, telemid, tracksamples*sizeof(double), "brake", brakedata);
    b = updatetelemetry(conn, telemid, tracksamples*sizeof(int), "rpms", rpmdata);
    b = updatetelemetry(conn, telemid, tracksamples*sizeof(int), "gear", geardata);
    b = updatetelemetry(conn, telemid, tracksamples*sizeof(int), "speed", speeddata);

    return b;
}
