## SensorHub
Compact HTTP-Server to collect temperature, humidity, and pressure data from several Sensor units using HTTP-POST-Requests and display the data by hosting a small Webserver.

This project features a small, custom HTTP server that interacts with several esp32-based sensor units which continuely send temp-,hum- and pressure-measurement data. After successfully receiving a sensor reading the server saves the data into a SQLite database. On receiving a GET-Request the server sends back the web interface to showcase the readings conveniently: 

![Web-Interface_for_Sensor_Readings](https://github.com/jzimme14/SensorHub/assets/98842597/1b3d7661-d793-403d-96a0-4098466f9990)

# How to run the server

# Building the Sensor Units
