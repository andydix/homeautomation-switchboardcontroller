# homeautomation-switchboardcontroller
## Switchboard controller Arduino sketch

This sketch is used to switch multiple relays in a mains switchboard using multiple [8-Channel Relay Driver Shields from Freetronics](https://www.freetronics.com.au/products/relay8-8-channel-relay-driver-shield#.WzejRNLdtPY "Freetronics' Website").

[MQTT](http://mqtt.org/) topics are used to determine which relay is switched and therefore which load within the house is turned on or off.

[Node-RED](https://nodered.org/) is used to orchestrate how each load in the house is switched.
