# SensorHub
Compact HTTP-Server to collect temperature, humidity, and pressure data from several Sensor units using HTTP-POST-Requests and display the data by hosting a small Webserver.

This project features a small, custom HTTP server that interacts with several esp32-based sensor units which continuely send temp-,hum- and pressure-measurement data. After receiving a sensor reading the server saves the data into a SQLite database. On receiving a GET-Request the server sends back the web interface to showcase the readings conveniently.


## How to run the server
At first, open a terminal and navigate to the folder you want to work in. 
Then clone this git repository using the following command: 

```
git clone https://github.com/jzimme14/SensorHub.git
```

To build the server and all necessary code use the compile script:

```
./compile.sh
```
The output file is called "sensorhub.out".

Now you are all set to start interacting with the server.

### Start Server as background-operation
Start the server:

```
./sensorhub.out
```
after starting the server, pause its operation by pressing CTRL+Z and send it to the background: 
```
bg
```

The server is now up and running in the background waiting for incoming connections.

### How to use the Web-Interface
To open the Web-Interface find out the ip address of the machine the server is running on. 

#port# = Port the server is listening on. (default: 8080)

Enter into the search bar:

_#ip-address#:#port#/index.html_

This is what the Website should look like:

![Web-Interface_for_Sensor_Readings](https://github.com/jzimme14/SensorHub/assets/98842597/1b3d7661-d793-403d-96a0-4098466f9990)

### Configure used Port
The server uses port 8080 by default. This can be adjusted as needed by looking for this line in the server.c file:

```
struct sockaddr_in addr = {AF_INET, 0x901F, 0};
```

To use, for example, port 12345:
__0d12345 -> 0x3039 -> flip digits = 0x3930__
Now change the port parameter: 

```
struct sockaddr_in addr = {AF_INET, 0x3930, 0};
```

Now the server listens on a connection at port 12345!

## Building the Sensorunits
A Sensor Unit contains just 3 simple components:

+ lolin32 or any comparable esp32-based mcu board
+ bme280 temperature, humidity and pressure sensor
+ battery able to provide 3.3v output (if output-voltage differs from 3.3V use a linear voltage regulator -> Plenty cheap options on AliExpress or other)

These components are soldered together according to the datasheets of the specific parts and the circuit diagram in this project. 

### Flashing/Initializing Sensorunits
The units have to be initialized by flashing the code and setting the specific Unit-ID. Every Sensorunit has its own ID which is set in the pio-project's code prior to flashing the chip. The Number given to a Sensorunit can be chosen independently. It is only used to identify the received data and assign it to a specific Sensorunit. 

Set the ID in line 39 of the main.cpp file in the lolin32_transponder_pio project folder. 
```
const int id = #specificID#;
```
