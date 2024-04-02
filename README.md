# SensorHub
Compact HTTP-Server to collect temperature, humidity, and pressure data from several Sensor units using HTTP-POST-Requests and display the data by hosting a small Webserver.

This project features a small, custom HTTP server that interacts with several esp32-based sensor units which continuely send temp-,hum- and pressure-measurement data. After receiving a sensor reading the server saves the data into a SQLite database. On receiving a GET-Request the server sends back the web interface to showcase the readings conveniently: 

![Web-Interface_for_Sensor_Readings](https://github.com/jzimme14/SensorHub/assets/98842597/1b3d7661-d793-403d-96a0-4098466f9990)

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

###Start Server as background-operation
Start the server:

```
./sensorhub.out
```
after starting the server, pause its operation by pressing CTRL+Z and send it to the background: 
```
bg
```

The server is now up and running in the background waiting for incoming connections.

## Building the Sensor Units
