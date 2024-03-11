// networking includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/sendfile.h>
// c includes
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <time.h>
// own file includes
#include "sqlite/sqlite3.h"

// dataframe to hold a data-stamp from a specific transponder (date_and_time is added by the server at connection time)
typedef struct {
	int transponderID;
	float temp;
	float hum;
	float pressure;
} dataframe;

// prototypes
 int saveDataToDatabase(dataframe d);


int main(void){
   printf("test started with db! \n");
   dataframe d = {3, 24.343, 65.4, 1058};
   saveDataToDatabase(d);

   while(1);






    // run variable
    bool running = true;

    // create special instance of sockaddr_in
    struct sockaddr_in addr = {
        AF_INET,
        0x901F,
        0
    };

    int s = socket(AF_INET,SOCK_STREAM,0);
    bind(s, &addr, sizeof(addr));

    while(running){
        listen(s, 10);
            
        int client_fd = accept(s, 0, 0);

        char buffer[256] = {0};
        recv(client_fd, buffer, 256,0);

	printf("%s", buffer);

	if(buffer[0] == 'G'){

        	// GET /file.html .......
        	char* f = buffer + 5;
        	*strchr(f, ' ') = 0;

        	// file handle of opened html-data
        	int opened_fd = open(f, O_RDONLY);

        	// send file to client
        	sendfile(client_fd, opened_fd, 0, 256);

        	// close file descriptor of data
        	close(opened_fd);

        	printf("%s send to client!\n",f);
	}
	else if(buffer[0] == 'P'){
		// get file name to which data is posted to
		char * f = buffer + 6;
		*strchr(f, ' ') = 0;
		
		// get data length
    		char *content_length_str = strstr(buffer, "Content-Length:");
    		if (content_length_str != NULL) {
        	// Move the pointer to the value part of the header
        	content_length_str += strlen("Content-Length:");

        	// Extract the content length as an integer
        	int content_length;
        	sscanf(content_length_str, "%d", &content_length);

        	printf("Content-Length: %d\n", content_length);
    		} else {
    	    		printf("Content-Length header not found\n");
    		}


		char buf[256];
		int buf_size = 256;
		read(client_fd, buf, buf_size);
		printf("read output: %s",buf);



		printf("\ntrying to write to %s\n", f);

    		int opened_fd = open(f, O_WRONLY | O_CREAT | O_APPEND, 0644);

		int wreturnval = write(opened_fd, buf, buf_size);
		printf("write-return-val: %d\n", wreturnval);
		
		
		char* response = "HTTP/1.1 200 OK\r\n";
		send(client_fd,response, strlen(response),0);

		close(opened_fd);
	}
	close(client_fd);
    }

    // close socket
    close(s);

    return 0;
}




int saveDataToDatabase(dataframe d){

   sqlite3 *db;
   char *zErrMsg = 0;
   int rc;
   char *sql;

   /* Open database */
   rc = sqlite3_open("measData.db", &db);
   
   if( rc ) {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return(0);
   } else {
      fprintf(stdout, "Opened database successfully\n");
   }

   /* Create Table for Measurements if it wasn't already created */
   const char* create_table_sql = "CREATE TABLE IF NOT EXISTS Measurements (MeasNumber INTEGER PRIMARY KEY, ID INTEGER, temp REAL, hum REAL, pressure REAL, date_time TEXT);";
   rc = sqlite3_exec(db, create_table_sql, 0,0,&zErrMsg);

   /* Put Data into Database */
   // Get Time from sqlite natively by using 'SELECT datetime('now','localtime')'
   char insert_data_sql[200];
   const char* insert_data_sql_blueprint = "INSERT INTO Measurements (ID, temp, hum, pressure, date_time) VALUES (%d, %.2f, %.2f, %.2f, datetime('now','localtime'));";
   sprintf(insert_data_sql, insert_data_sql_blueprint, d.transponderID, d.temp, d.hum, d.pressure);
   rc = sqlite3_exec(db, insert_data_sql,0,0,&zErrMsg);


   if( rc ) {
       fprintf(stderr, "cant add data: %s\n", sqlite3_errmsg(db));
       return(0);
   } else {
       fprintf(stdout, "added data successfully\n");
   }


return 1;
}
