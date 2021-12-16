# Garage Mongoose OS app

# Hardware Platform
ESP8266 LoLin NodeMCU. 

<img src="docs/ESP8266-NodeMCU-kit-12-E-pinout-gpio-pin.png">

It's important to select the correct PINs for the relays so that you don't get cycling as the device resets, powers on, etc. The relays assume they are controlled from a pin pulled high. In other words, they switched on by transitioning their pin from HI to LO.

In this case, I'm using GPIO 4 & 5 to control the door motor activation.

## Wiring Legend
Device                | Silkscreen | Logical Name 
------------          | ---------- | -------------
DHT22 Temp/RH         | D3         | GPIO0
Door A Closed Contact | D6         | GPIO12
Door A Activation     | D4         | GPIO2
Door B Activation     | ??         | ??

In this case, I'm using GPIO 4 to control the south door relay.

## DHT22 Sensor
The DHT22 is a low-cost temperature and humidity sensor. Although the DHT22 sensor comes with 4 pins, only 3 are connected:
<img src="./docs/DHT22-Sensor-Pinout.png">


# Features
 * Relay cycle control for garage door opening
 * DHT22 temp/humidity readings
 * MQTT publishing
 * HTTP/MQTT RPC Control
 * Second relay control for lights
 * OTA Firmware Updating

 # Relay Activation
 Relay activation simulates a doorbell button press, the relay turns on briefly, then is turned off. This switching on and then off is in response to a single activation request from the user, as opposed to requiring 2 user requests, one to turn the relay on followed by another one to turn it off. 
 ## HTTP
 Issue an HTTP get like so, no paylaod is required. 
 ```bash
 curl http://192.168.1.xxx/rpc/SouthDoor.Activate
 ```

 ## MQTT RPC
 you can send a command via MQTT. This means you don't need to have a static IP for the device, but you will need to know its unique ID. I'm working on enabling auto-discovery in services like OpenHAB by leveraging via the [homie convention](https://homieiot.github.io).

In the example below, "alpha" is "correlation ID", that is, an identifier that will be echoed back by the node in response to an RPC request which will contain information about the node's response, errors, etc. 

 ```bash
 mqttpub -h 192.168.1.5 \
    -t esp8266_7E5E96/rpc  \
    '{id: 1, "src":"alpha", "method": "SouthDoor.Activate"}' 
 ```

## MQTT Trigger
TODO: It would be simpler in (especially for OpenHAB) if the relay activation could be triggered simply by the arrival of any message body `1` to a set topic, rather than having to craft and send a JSON document per MongooseOS' RPC requirements.


# OTA Firmware Update
When you use the `mos flash` command certain configurations parameters (like WiFi) configuration are lost.

The simplest method is to use an HTTP POST:
```bash
cd project-repo-root
curl -i -F filedata=@./build/fw.zip http://192.168.1.xxx/update
```