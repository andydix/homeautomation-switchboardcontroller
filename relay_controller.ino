/*
	Hardware setup is an Arduino Uno with an Ethernet 2 shield plus multiple
	8-Channel Relay Driver Shields from Freetronics.
		https://www.freetronics.com.au/products/relay8-8-channel-relay-driver-shield
		https://www.freetronics.com.au/pages/relay8-8-channel-relay-driver-shield-quickstart-guide

	MQTT messages published to the topic switchboard/# are converted to a board/channel index combination
	then used to update the byte array of relay states. The relevent states are then sent to
	the correct board to switch those channels accordingly.

	e.g. sending ascii 48 to topic switchboard/42 will activate channel 2 on board 6
*/

#include <Ethernet2.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Wire.h>

// MAC address of the Ethernet controller
// ! Newer Ethernet shields have a MAC address printed on a sticker on the shield !
byte mac[] = {0x90, 0xA2, 0xDA, 0x10, 0x04, 0x7E};

// address of the MQTT broker
const char *msgBroker = "192.168.0.2";

// initialise Ethernet and MQTT clients
EthernetClient ethClient;
PubSubClient msgClient(ethClient);

// board 1 - relays 1 to 8
// board 2 - relays 9 to 16
// board 3 - relays 17 to 24
// board 4 - relays 25 to 32
// board 5 - relays 33 to 40
// board 6 - relays 41 to 48
// board 7 - relays 49 to 56
// board 8 - relays 57 to 64
const byte I2C_ADDR[] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27};
byte relayStates[8] = {0, 0, 0, 0, 0, 0, 0, 0};

void setup()
{
	// open serial port for debug messages
	Serial.begin(115200);
	Serial.println(F("\n## Smart Home Switchboard Controller ##\n"));

	// start the Ethernet connection
	setup_ethernet();

	// wake up I2C bus
	Wire.begin();

	// initialise each relay board
	init_relay8s();

	// MQTT setup
	Serial.println(F("MQTT Setup"));
	msgClient.setServer(msgBroker, 1883);
	msgClient.setCallback(mqttCallback);
} // setup

void setup_ethernet()
{
	Serial.println(F("Network Setup"));
	Serial.print(F("Getting a network address using DHCP... "));
	if (Ethernet.begin(mac) == 0)
	{
		// no point in carrying on, so do nothing forever
		Serial.println(F("! failed to get a network address !"));
		Serial.println(F("!! PROGRAM STOPPED !!"));
		for (;;)
			;
	}
	Serial.println(F("connected"));
	Serial.print(F("Network address given is "));
	Serial.println(Ethernet.localIP());
	Serial.println();
} // setup_ethernet

void init_relay8s()
{
	Serial.println(F("Relay Driver Boards Setup"));
	Serial.print(F("Initialising board... "));
	for (int i = 0; i < 8; i++)
	{
		Serial.print(F(" #"));
		Serial.print(i);
		Wire.beginTransmission(I2C_ADDR[i]);
		Wire.write(0x00); // IODIRA register
		Wire.write(0x00); // Set all of bank A to outputs
		Wire.endTransmission();
		Serial.print(F(" OK"));
	}
	Serial.print(F("\n\n"));
} // init_relay8s

void loop()
{
	// check to see if the MQTT client is connected to the broker
	if (!msgClient.connected())
	{
		// if not, try to re-establish the connection
		reconnect();
	}
	// give the MQTT client time to process messages
	msgClient.loop();
} // loop

void reconnect()
{
	// loop until connected to the MQTT broker
	while (!msgClient.connected())
	{
		Serial.print(F("Contacting the MQTT broker... "));
		// attempt to connect
		if (msgClient.connect("switchboardClient"))
		{
			Serial.println("broker replied OK");
			// subscribe to all messages for the switchboard
			msgClient.subscribe("switchboard/#");
			Serial.println(F("Subscribed to topic 'switchboard/#'\n"));
		}
		else
		{
			Serial.print(F("broker is not replying; error code is "));
			Serial.print(msgClient.state());
			// wait 5 seconds before retrying
			Serial.println(F("... trying again in 5 seconds..."));
			delay(5000);
		}
	}
} // reconnect

// callback function to receive subscribed MQTT messages
// assume topic is always switchboard/n where n
// equals the number of the relay to switch and the
// payload is 1 for on and 0 for off
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
	Serial.print(F("["));
	Serial.print(topic);
	Serial.print(F("] "));
	Serial.println(payload[0]);

	// get the relay number from the topic string
	char *p = strstr(topic, "/") + 1;
	int relayNumber = atoi(p);

	// convert the relay number into a board & channel index
	// e.g. relayNumber 20 = board 3 channel 4
	int board = relayNumber / 8;
	int channel = relayNumber % 8;
	if (channel == 0)
		channel = 8;
	else
		board++;

	// and decrement by 1 as both need to be zero based
	board--;
	channel--;

	// 48 = 0, 49 = 1 - 0 = off, 1 = on
	if (payload[0] == 48) // 0 = off
		bitClear(relayStates[board], channel);
	if (payload[0] == 49) // 1 = on
		bitSet(relayStates[board], channel);

	setRelayStates(board, relayStates[board]);
} // mqttCallback

void setRelayStates(int board, int states)
{
	Wire.beginTransmission(I2C_ADDR[board]);
	// Select GPIOA
	Wire.write(0x12);
	// Send value to bank A
	Wire.write(states);
	Wire.endTransmission();
} // setRelayStates