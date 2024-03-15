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
	char type[10];
	int content_length;
	char connection[20];
	char content_type[20];
	char target_file[20];
	char *payload;
} HTTP_Request;

// prototypes
int saveDataToDatabase(dataframe d);
HTTP_Request http_request_constr(char *buffer);

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
		HTTP_Request http_request = http_request_constr(buffer);

		printf("type: %s\ncontent-length: %d\nconnection: %s\ncon-type: %s \ntarget-dir: %s \n", http_request.type, http_request.content_length, http_request.connection, http_request.content_type, http_request.target_file);
		printf("\n\nbuffer: %s\n\n", buffer);

		if (strcmp(http_request.type, "GET") == 0)
		{
			printf("entered GET section\n");

			// file handle of opened html-data
			int opened_fd = open(f, O_RDONLY);
			printf("file to read: %s\n", f);

			// send file to client
			sendfile(client_fd, opened_fd, 0, 256);

			// close file descriptor of data
			close(opened_fd);

			printf("%s send to client!\n", f);
		}
		else if (strcmp(http_request.type, "POST") == 0)
		{
			http_request.payload = malloc(http_request.content_length);
			printf("payload buffer size(bytes): %d\n", http_request.content_length);
			read(client_fd, http_request.payload, http_request.content_length);
			printf("read output: %s", http_request.payload);

			// Testing on demand db-filling
			dataframe d = {4, 232.3, 64.9434, 23.343};
			saveDataToDatabase(d);

			printf("\ntrying to write to %s\n", http_request.target_file);

			char *response = "HTTP/1.1 200 OK\r\n";
			send(client_fd, response, strlen(response), 0);

			free(http_request.payload);
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
HTTP_Request http_request_constr(char *buffer)
{
	HTTP_Request h = {"", 0, ""};

	if (strstr(buffer, "POST"))
	{
		strcpy(h.type, "POST");
	}
	else if (strstr(buffer, "GET"))
	{
		strcpy(h.type, "GET");
	}

	int buflen = strlen(buffer);

	char *ubuf = malloc(buflen);
	strcpy(ubuf, buffer);

	// get content-length
	char *clptr = strstr(ubuf, "Content-Length:");
	if (clptr != NULL)
	{
		clptr = clptr + 16;
		*strchr(clptr, '\n') = 0;
		h.content_length = *clptr;
	}
	// else
	// {
	// 	h.content_length = 0;
	// }
	free(ubuf);

	char *ubuf1 = malloc(buflen);
	strcpy(ubuf1, buffer);

	// connection type
	char *conptr = strstr(ubuf1, "Connection:");
	if (conptr != NULL)
	{
		conptr = conptr + 12;
		*strchr(conptr, '\n') = 0;
		strcpy(h.connection, conptr);
	}
	// else
	// {
	// 	h.connection = "";
	// }
	free(ubuf1);

	char *ubuf2 = malloc(buflen);
	strcpy(ubuf2, buffer);
	char *contypeptr = strstr(ubuf2, "Content-Type: ");
	if (contypeptr != NULL)
	{
		contypeptr += 14;
		*strchr(contypeptr, '\n') = 0;
		strcpy(h.content_type, contypeptr);
	}
	free(ubuf2);

	char *ubuf3 = malloc(buflen);
	strcpy(ubuf3, buffer);

	char *tardatptr = ubuf3;
	if (strcmp(h.type, "POST") == 0)
	{
		tardatptr += 6;
	}
	else if (strcmp(h.type, "GET") == 0)
	{
		tardatptr += 5;
	}

	*strchr(tardatptr, ' ') = 0;
	strcpy(h.target_file, tardatptr);
	free(ubuf3);

	return h;
}
