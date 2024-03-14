// networking includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/sendfile.h>
// c includes
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <time.h>
// own file includes
#include "sqlite/sqlite3.h"

// dataframe to hold a data-stamp from a specific transponder (date_and_time is added by the server at connection time)
typedef struct
{
	int transponderID;
	float temp;
	float hum;
	float pressure;
} dataframe;

typedef struct
{
	char *type;
	int content_length;
	char *connection;
} HTTP_Request;

// prototypes
int saveDataToDatabase(dataframe d);
HTTP_Request fill_http_request_obj(char *buffer);

int main(void)
{
	// printf("test started with db! \n");
	// dataframe d = {3, 24.343, 65.4, 1058};
	// saveDataToDatabase(d);

	//   while(1);

	// run variable
	bool running = true;

	// create special instance of sockaddr_in
	struct sockaddr_in addr = {
		AF_INET,
		0x901F,
		0};

	int s = socket(AF_INET, SOCK_STREAM, 0);
	bind(s, &addr, sizeof(addr));

	while (running)
	{
		listen(s, 10);

		int client_fd = accept(s, 0, 0);

		char buffer[256] = {0};
		recv(client_fd, buffer, 256, 0);

		// http header information interface
		HTTP_Request http_request = fill_http_request_obj(buffer);

		printf("type: %s, content-length: %d, connection: %s", http_request.type, http_request.content_length, http_request.connection);
		printf("\n\nbuffer: %s\n\n", buffer);

		if (http_request.type == "GET")
		{

			// GET /file.html .......
			char *f = buffer + 5;
			*strchr(f, ' ') = 0;

			// file handle of opened html-data
			int opened_fd = open(f, O_RDONLY);
			printf("file to read: %s\n", f);

			// send file to client
			sendfile(client_fd, opened_fd, 0, 256);

			// close file descriptor of data
			close(opened_fd);

			printf("%s send to client!\n", f);
		}
		else if (http_request.type == "POST")
		{
			// get file name to which data is posted to
			char *f = buffer + 6;
			*strchr(f, ' ') = 0;

			int payload_bufsize = http_request.content_length;
			char *payload_buf = malloc(payload_bufsize);
			printf("\ncontent length: %s, payload buffer size(bytes): %d", http_request.content_length, payload_bufsize);
			read(client_fd, payload_buf, payload_bufsize);
			printf("read output: %s", payload_buf);

			// Testing on demand db-filling
			dataframe d = {4, 232.3, 64.9434, 23.343};
			saveDataToDatabase(d);

			printf("\ntrying to write to %s\n", f);

			char *response = "HTTP/1.1 200 OK\r\n";
			send(client_fd, response, strlen(response), 0);
		}
		close(client_fd);
	}

	// close socket
	close(s);

	return 0;
}

int saveDataToDatabase(dataframe d)
{

	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	char *sql;

	/* Open database */
	rc = sqlite3_open("measData.db", &db);

	if (rc)
	{
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		return (0);
	}
	else
	{
		fprintf(stdout, "Opened database successfully\n");
	}

	/* Create Table for Measurements if it wasn't already created */
	const char *create_table_sql = "CREATE TABLE IF NOT EXISTS Measurements (MeasNumber INTEGER PRIMARY KEY, ID INTEGER, temp REAL, hum REAL, pressure REAL, date_time TEXT);";
	rc = sqlite3_exec(db, create_table_sql, 0, 0, &zErrMsg);

	/* Put Data into Database */
	// Get Time from sqlite natively by using 'SELECT datetime('now','localtime')'
	char insert_data_sql[200];
	const char *insert_data_sql_blueprint = "INSERT INTO Measurements (ID, temp, hum, pressure, date_time) VALUES (%d, %.2f, %.2f, %.2f, datetime('now','localtime'));";
	sprintf(insert_data_sql, insert_data_sql_blueprint, d.transponderID, d.temp, d.hum, d.pressure);
	rc = sqlite3_exec(db, insert_data_sql, 0, 0, &zErrMsg);

	if (rc)
	{
		fprintf(stderr, "cant add data: %s\n", sqlite3_errmsg(db));
		return (0);
	}
	else
	{
		fprintf(stdout, "added data successfully\n");
	}

	return 1;
}

// Blueprint for a http-header. Is filled after receiving raw header string. Used to access specific Header-Specifiers
HTTP_Request fill_http_request_obj(char *buffer)
{
	HTTP_Request h;

	if (strstr(buffer, "POST"))
	{
		h.type = "POST";
	}

	if (strstr(buffer, "GET"))
	{
		h.type = "GET";
	}

	// get content-length
	char *buf1 = NULL;
	strcpy(buf1, buffer);
	char *clptr = strstr(buf1, "Content-Length:");
	if (clptr != NULL)
	{
		clptr = clptr + 16;
		*strchr(clptr, '\n') = 0;
		h.content_length = *clptr;
	}
	else
	{
		h.content_length = 0;
	}

	// connection type
	char *buf2 = NULL;
	strcpy(buf2, buffer);
	char *conptr = strstr(buf2, "Connection:");
	if (conptr != NULL)
	{
		conptr = conptr + 12;
		*strchr(conptr, '\n') = 0;
		strcpy(h.connection, conptr);
	}
	else
	{
		h.connection = "";
	}

	return h;
}
