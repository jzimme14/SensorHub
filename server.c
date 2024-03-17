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

#define _GNU_SOURCE

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
void print_visible_characters(const char *str);
dataframe ReceivedData_into_Dataframe(char *s);

int main(void)
{
	// run variable
	bool running = true;

	// create special instance of sockaddr_in
	const struct sockaddr_in addr = {AF_INET, 0x901F, 0};

	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (bind(socket_fd, &addr, sizeof(addr)) != 0)
	{
		fprintf(stderr, "bind failed...");
		exit(-3);
	}

	while (running)
	{
		listen(socket_fd, 10);

		int client_fd = accept(socket_fd, 0, 0);

		char buffer[256] = {0};
		if (recv(client_fd, buffer, 256, 0) == -1)
		{
			fprintf(stderr, "recv failed...");
			exit(-4);
		}

		// http header interface
		HTTP_Request http_request = http_request_constr(buffer);
		// print_visible_characters(buffer);
		printf("type: %s\ncontent-length: %d\nconnection: %s\ncon-type: %s \ntarget-dir: %s \n\n", http_request.type, http_request.content_length, http_request.connection, http_request.content_type, http_request.target_file);

		if (strcmp(http_request.type, "GET") == 0)
		{
			printf("entered GET section\n");

			if (strcmp(http_request.target_file, "close_socket") == 0)
			{
				printf("\nsocket is closed! \n");
				running = false;
			}
			else
			{
				// file handle of opened html-data
				int opened_fd = open(http_request.target_file, O_RDONLY);
				printf("file to read: %s\n", http_request.target_file);

				// send file to client
				sendfile(client_fd, opened_fd, 0, 256);

				// close file descriptor of data
				close(opened_fd);
			}
		}
		else if (strcmp(http_request.type, "POST") == 0)
		{
			http_request.payload = malloc(http_request.content_length);
			if (http_request.payload == NULL)
			{
				fprintf(stderr, "malloc failed for http_req.payload...");
				exit(-6);
			}
			if (read(client_fd, http_request.payload, http_request.content_length) == -1)
			{
				fprintf(stderr, "reading POST-payload failed...");
				exit(-7);
			}
			printf("read output: %s\n", http_request.payload);

			// Put received data into fitting dataframe/format
			dataframe payload;
			memset(&payload, 0, sizeof(dataframe));
			payload = ReceivedData_into_Dataframe(http_request.payload);

			// save data to sqlite-database
			saveDataToDatabase(payload);

			char *response = "HTTP/1.1 200 OK\r\nServer: butzdigga\r\nConnection: close\r\n";
			send(socket_fd, response, strlen(response), 0);

			free(http_request.payload);
		}
		else
		{
			fprintf(stderr, "\naccessed using unknown http-request...");
		}
		// close client connection
		if (close(client_fd) != 0)
		{
			fprintf(stderr, "closing client connection failed...");
			exit(-5);
		}
	}

	// close socket
	if (close(socket_fd) != 0)
	{
		fprintf(stderr, "closing socket failed...");
	}

	return 0;
}

dataframe ReceivedData_into_Dataframe(char *s)
{
	dataframe d;
	memset(&d, 0, sizeof(dataframe));

	int retVal = sscanf(s, "%d,%f,%f,%f", &d.transponderID, &d.temp, &d.hum, &d.pressure);
	if (retVal == 4)
	{
		return d;
	}
	else
	{
		fprintf(stderr, "read error occured! ");
		exit(11);
	}
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

	sqlite3_close(db);

	return 0;
}

// Blueprint for a http-header. Is filled after receiving raw header string. Used to access specific Header-Specifiers
HTTP_Request http_request_constr(char *buffer)
{
	HTTP_Request h;
	memset(&h, 0, sizeof(HTTP_Request));

	if (strstr(buffer, "POST"))
	{
		strcpy(h.type, "POST");
	}
	else if (strstr(buffer, "GET"))
	{
		strcpy(h.type, "GET");
	}

	// get length of buffer
	int buflen = strlen(buffer);
	buflen++;

	// get content-length
	char *ubuf = malloc(buflen);
	if (ubuf != NULL)
	{
		strcpy(ubuf, buffer);
		char *clptr = strstr(ubuf, "Content-Length:");
		if (clptr != NULL)
		{
			clptr = clptr + 16;
			char *endptr = strchr(clptr, '\r');
			if (endptr != NULL)
			{
				// Nullterminiere den String, damit atoi nur die Zahl liest
				*endptr = '\0';
				// Konvertiere die Zahl in einen Integer
				h.content_length = atoi(clptr);
			}
		}
	}
	else
	{
		h.content_length = 0;
	}
	free(ubuf);

	// connection type
	char *ubuf1 = malloc(buflen);
	if (ubuf1 != NULL)
	{
		strcpy(ubuf1, buffer);
		char *conptr = strstr(ubuf1, "Connection:");
		if (conptr != NULL)
		{
			conptr = conptr + 12;
			*strchr(conptr, '\r') = 0;
			strcpy(h.connection, conptr);
		}
	}
	else
	{
		strcpy(h.connection, " ");
	}
	free(ubuf1);

	// get connection type
	char *ubuf2 = malloc(buflen);
	if (ubuf2 != NULL)
	{
		strcpy(ubuf2, buffer);
		char *contypeptr = strstr(ubuf2, "Content-Type: ");
		if (contypeptr != NULL)
		{
			contypeptr += 14;
			*strchr(contypeptr, '\r') = 0;
			strcpy(h.content_type, contypeptr);
		}
	}
	free(ubuf2);

	// get target directory
	char *ubuf3 = malloc(buflen);
	if (ubuf3 != NULL)
	{
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
	}
	free(ubuf3);

	return h;
}

void print_visible_characters(const char *str)
{
	// Gehe durch jeden Zeichen im String
	for (int i = 0; str[i] != '\0'; i++)
	{
		// Überprüfe, ob das Zeichen ein unsichtbares Zeichen ist
		if (str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r')
		{
			// Gib das unsichtbare Zeichen als Escape-Sequenz aus
			switch (str[i])
			{
			case ' ':
				printf("' ' (Space)\n");
				break;
			case '\t':
				printf("\\t (Tab)\n");
				break;
			case '\n':
				printf("\\n (Newline)\n");
				break;
			case '\r':
				printf("\\r (Carriage Return)\n");
				break;
			// Füge weitere unsichtbare Zeichen nach Bedarf hinzu
			default:
				printf("Unknown invisible character\n");
				break;
			}
		}
		else
		{
			// Gib sichtbare Zeichen direkt aus
			printf("%c\n", str[i]);
		}
	}
}
