#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "sqlite/sqlite3.h"

int getDataFromDatabase(int transponderID, char *resultString)
{
    sqlite3 *db;
    sqlite3_stmt *stmt;

    char sql_query[300];
    memset(sql_query, 0, sizeof(sql_query));

    char result[50];
    memset(result, 0, sizeof(result));

    // Öffne die Datenbankverbindung
    int rc = sqlite3_open("measData.db", &db);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    sprintf(sql_query, "SELECT * FROM Measurements WHERE ID = %d ORDER BY date_time DESC LIMIT 1;", transponderID);

    rc = sqlite3_prepare_v2(db, sql_query, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    double temp = 0, hum = 0, pressure = 0;
    const unsigned char *date_time;

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        temp = sqlite3_column_double(stmt, 2);
        hum = sqlite3_column_double(stmt, 3);
        pressure = sqlite3_column_double(stmt, 4) / 100; // convert from Pa -> hPa
        date_time = sqlite3_column_text(stmt, 5);
    }
    else
    {
        fprintf(stderr, "error while reading database...");
        exit(-10);
    }

    sprintf(result, "%d,%.2f,%.2f,%.2f,%s", transponderID, temp, hum, pressure, date_time);

    // Schließe die SQL-Abfrage und die Datenbankverbindung
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    strcpy(resultString, result);

    return 0;
}

int main(void)
{
    char r[100];
    memset(r, 0, sizeof(r));
    getDataFromDatabase(1, r);
}