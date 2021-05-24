# Smart Bathtub

## Getting started
This project needs Pistache and Paho binaries in order to build and run.

Copy the environment example and change things if necessary:

```
cd src
cp env.hpp.example env.hpp
```
After setting the environment, you can build the app.
```
make build
```
To run the app, you will also need a MQTT server:
```
apt-get install mosquitto
mosquitto -v
```
If you don't want to use a local server, you can try using a public test server. In the `env.hpp` file, use this line:
```
#define MQTT_SERVER_ADDRESS "tcp://broker.emqx.io:1883"
```
Open another terminal and run the app:
```
make run
```

## View traffic on the MQTT network
Mosquitto verbose mode is cool, but it doesn't show message payloads.
You can better see what messages are send over the network by subscribing to all the topics this app uses.
Open a new terminal window and run:
```
mosquitto_sub -t "temperature" -t "waterQuality" -t "salt" -t "display"
```

## Using `mosquitto_pub` to simulate sensor messages
Example for waterQuality:
```
mosquitto_pub -t "waterQuality" -m "7,300,0.2,300,20"
```
This will send the water quality parameters to the smart bath and will make pipes stop since the calcium levels of the water are too high.

**Note:** If you are not running the MQTT server locally, you should add `-h broker.emqx.io` to the command above.

### The "display"
We've made a frontend React app that uses web sockets to communicate with the SmartBath.
[Check it out](frontend)

## HTTP Requests
We made a shared Postman collection that will help you play with the app.
You will need to Import the collection in your Postman app by clicking Import > Link and pasting `https://www.postman.com/collections/a476e81cd402a7fd3314`.

## Report and Buffer Specification
 - [Raport de analiză](docs/Raport%20de%20analiza.pdf)
 - [Buffer specs](docs/buffers.json)

# References
1. sciencing.com, "How to Calculate the Volume of a Person" (accessed 5/18/2021) - https://sciencing.com/calculate-volume-person-7853815.html
