#include "RealTimeClock.h"
#include "Config.h"
#include "WebConfig.h"
#include "PulsePort.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include "MBus.h"


/*
 * Buffers to hold pointers in global scope 
 */
char BUFFER_MQTT_CLIENT[sizeof(Adafruit_MQTT_Client)];		// Buffer for gMqttClient
char BUFFER_MQTT_PUBLISH[sizeof(Adafruit_MQTT_Publish)];	// Buffer for gMqttPublish
char BUFFER_WIFICLIENT[sizeof(WiFiClient)];					// Buffer for gWebClient
char BUFFER_ELECTRICAL_METER[sizeof(PulsePort)];			// Buffer for gElectricalMeter
char BUFFER_HEAT_PUMP_METER[sizeof(PulsePort)];				// Buffer for gHeatPumpMeter
char BUFFER_CLOCK[sizeof(RealTimeClock)];					// Buffer for gClock


/*
 * Application Constants
 */
const unsigned long REPORT_INTERVAL = 300000;	// 60000ms == 1 minute delay between MQTT reports
const byte EEPROM_CHECK_SUM = 124;				// Used to check if config is stored. Change if structure changes
const int EEPROM_CONFIG_ADDRESS = 0;			// EEPROM address where config class is stored
const int AP_PIN = 2;							// Input pin for Access Point mode
const int GREEN_LED = 4;						// Green LED on Pin 4
const int RED_LED = 5;							// Red LED on Pin 5
												
/*
 * Global variables
 */
bool gIsAccessPointMode = false;				// Flag to see if we're running in config mode (Access Point) or normal
Config gAppConfig;								// The configuration, as set in EEPROM from the web config page
RealTimeClock* gClock;							// An instance of a clock, used to keep track of current time
WiFiClient* gWiFiClient;						// WiFi client, used by the MQTT stuff
Adafruit_MQTT_Client* gMqttClient;				// MQTT client, used to connect to a MQTT server
Adafruit_MQTT_Publish* gMqttChannel;			// MQTT channel, used to report data to the MQTT server
PulsePort* gElectricalMeter;					// The port used to read the electrical meter
PulsePort* gHeatPumpMeter;						// The port used to read the heat pump meter

/*
* Application Logic
*/
bool APButtonPressed();									// Test if the user is currently pressing the Access Point button
bool ConnectToWiFi();									// Used to connect to your access point
float ToPercentage(int pValue, int pFull, int pMin);	// Convert a number to a percentage between a min and a max
void SetupLEDs();										// Define the LED ports as outputs
void LED(int pPin, bool pOn);							// Signal LED on or off
void FlashLED(int pPin, int pCycles);					// Flash the LED a number of times

/*
* Web Configuration
*/
void SetupConfigServer();							// Setup the web server to handle the web config
WebConfig* gConfigServer;							// The web server itself
String HandleSavePage(ESP8266WebServer pServer);	// Serving the request to save configuration
String GetConfigPage();								// Serving the request to show/edit configuration

/*
* MQTT setup and reporting
*/
void SetupMqtt();													// Setup the MQTT connection
void ConnectMqtt();													// Connect to the MQTT server
void ReportToMqtt(PulsePort* pMeter);								// Set up the JSON and report data
void GetMqttTopic(char* pBuffer);									// Get the name of the MQTT topic
void GetJSON(char* pBuffer, int pBufferSize, PulsePort* pMeter);	// Produce the JSON to be reported

/*
* IO setup
*/
void SetupPorts();


void setup() 
{
	SetupLEDs();

	//noInterrupts();
	LED(GREEN_LED, true);
	LED(RED_LED, true);
	FlashLED(GREEN_LED, 3);
	FlashLED(RED_LED, 3);
	Serial.begin(115200);

	//gAppConfig = new Config();
	LED(GREEN_LED, false);
	LED(RED_LED, true);

	// If we have no stored config or manually requested, we'll start as an Access Point
	if (APButtonPressed() || !gAppConfig.Load(EEPROM_CONFIG_ADDRESS))
	{
		LED(RED_LED, true);
		LED(GREEN_LED, true);
		SetupConfigServer();
	}
	
	// We have config, so let's setup stuff and get on going
	else
	{
		Serial.println(F("Starting Setup..."));
		FlashLED(RED_LED, 1);
		LED(RED_LED, true);
		ConnectToWiFi();
		FlashLED(RED_LED, 1);
		LED(RED_LED, true);
		SetupMqtt();
		FlashLED(RED_LED, 1);
		LED(RED_LED, true);
		SetupClock();
		FlashLED(RED_LED, 1);
		LED(RED_LED, true);
		SetupPorts();
		FlashLED(RED_LED, 3);
		FlashLED(GREEN_LED, 3);
		Serial.println(F("Setup completed!"));
		delay(1000);
		LED(GREEN_LED, true);
		//interrupts();
	}
}

void loop() {
	if (gIsAccessPointMode)
	{
		// Handle web requests for configuration
		gConfigServer->Handle();
	}
	else
	{
		LED(GREEN_LED, true);

		// Make sure we have the current time
		if (!gClock->Update())
		{
			Serial.println(F("It's been too long since last time update. We will wait a fec seconds and try again later"));
			delay(10000);
			return;
		}

		// Report data to MQTT
		ReportToMqtt(gElectricalMeter);
		ReportToMqtt(gHeatPumpMeter);

		// wait a period, for the next reporting
		unsigned long vDelay = 0;
		while (vDelay < REPORT_INTERVAL)
		{
			LED(GREEN_LED, true);
			delay(300);
			LED(GREEN_LED, false);
			delay(4700);
			vDelay += 5000;
			
			gElectricalMeter->UpdateAverage();
			gHeatPumpMeter->UpdateAverage();
		}
	}
}
void SetupConfigServer()
{
	Serial.println(F("Setting up configuration web site..."));
	gConfigServer = new WebConfig(GetConfigPage, HandleSavePage);
	gConfigServer->Setup();
	gIsAccessPointMode = true;
}
float ToPercentage(int pValue, int pFull, int pMin)
{
	int vActualValue = pValue - pMin;
	return (float)vActualValue / (float)(pFull - pMin) * 100.0;
}
void SetupClock()
{
	Serial.print(F("Setting up clock (NTC client)"));
	gClock = new (BUFFER_CLOCK)RealTimeClock();
}
void SetupMqtt()
{
	Serial.print(F("Setting up MQTT for "));
	Serial.println(gAppConfig.MqttServer);

	gWiFiClient = new (BUFFER_WIFICLIENT)WiFiClient();
	gMqttClient = new (BUFFER_MQTT_CLIENT)Adafruit_MQTT_Client(gWiFiClient, gAppConfig.MqttServer, 1883, "", "");
	
	char* vTopic = new char[100];
	GetMqttTopic(vTopic);
	Serial.print(F("MQTT topic is "));
	Serial.println(vTopic);

	gMqttChannel = new (BUFFER_MQTT_PUBLISH)Adafruit_MQTT_Publish(gMqttClient, vTopic);
	ConnectMqtt();
}

void GetMqttTopic(char* pBuffer)
{
	strcpy(pBuffer, gAppConfig.MqttTopic);
	pBuffer[strlen(gAppConfig.MqttTopic)] = '/';
	pBuffer[strlen(gAppConfig.MqttTopic) + 1] = 0;
	strcat(pBuffer, gAppConfig.MqttName);
	pBuffer[strlen(gAppConfig.MqttTopic) + 1 + strlen(gAppConfig.MqttName)] = 0;
}

void GetJSON(char* pBuffer, int pBufferSize, PulsePort* pMeter)
{

	pMeter->UpdateAverage();

	unsigned long vCurrentTime = gClock->GetTime();

	/*
	Serial.print("Creating JSON buffer: ");
	Serial.print(JSON_OBJECT_SIZE(12));
	Serial.println(" bytes");
	*/

	StaticJsonBuffer<JSON_OBJECT_SIZE(8)> vJsonBuffer;
	JsonObject& vRoot = vJsonBuffer.createObject();
	vRoot["s"] = pMeter->Name;
	vRoot["t"] = vCurrentTime;
	vRoot["pwr"] = pMeter->TicksInWattHours();
	vRoot["tot"] = pMeter->TotalTicksInWattHours() + pMeter->TicksInWattHours();
	vRoot["avg"] = pMeter->GetAverage();
	vRoot["cur"] = pMeter->Current();
	vRoot["b"] = ToPercentage(analogRead(A0), 1024, 512);
	vRoot.printTo(pBuffer, pBufferSize);
}

void ReportToMqtt(PulsePort* pMeter)
{
	ConnectMqtt();
	
	char vMessage[150];
	GetJSON(vMessage, sizeof(vMessage), pMeter);

	if (!gMqttChannel->publish(vMessage))
	{
		Serial.println(F("Failed to report to MQTT"));
		Serial.println(vMessage);
	}
	else
	{
		Serial.print(F("Successfully reported to MQTT: "));
		Serial.println(vMessage);

		// Increase total values and reset counter
		pMeter->CommitTicksToTotal();
		
		// Store the values to EEPROM
		pMeter->SaveTotalValue();
	}

	// ping the server to keep the mqtt connection alive 
	if (!gMqttClient->ping()) {
		gMqttClient->disconnect();
	}
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void ConnectMqtt() {
	int8_t ret;

	// Stop if already connected.
	if (gMqttClient->connected()) {
		return;
	}

	Serial.print(F("Connecting to MQTT... "));
	//delay(5000);

	while ((ret = gMqttClient->connect()) != 0) { // connect will return 0 for connected
		Serial.println(F("MQTT::connect failed"));
		Serial.println(ret);
		Serial.println(gMqttClient->connectErrorString(ret));
		Serial.println(F("Retrying MQTT connection in 5 seconds..."));
		gMqttClient->disconnect();
		delay(5000);  // wait 5 seconds
	}
	Serial.println(F("MQTT Connected!"));
}


bool ConnectToWiFi()
{
	Serial.print(F("Connecting to AP: "));
	Serial.print(gAppConfig.Ssid);

	WiFi.softAPdisconnect(true);
	WiFi.disconnect(true);
	WiFi.mode(WIFI_OFF);
	delay(1000);

	WiFi.mode(WIFI_STA);
	WiFi.begin(gAppConfig.Ssid, gAppConfig.Pwd);

	while (WiFi.status() != WL_CONNECTED) 
	{
		FlashLED(RED_LED, 1);
		Serial.print(".");
		FlashLED(GREEN_LED, 1);
		delay(200);                       // Wait for another while (to demonstrate the active low LED)
		Serial.print(".");
	}
	FlashLED(GREEN_LED, 3);

	Serial.println("");
	Serial.print(F("Connected to WiFi at IP: "));
	Serial.println(WiFi.localIP());

	return true;
}

bool APButtonPressed()
{
	pinMode(AP_PIN, INPUT);

	Serial.print(F("Press AP button now to set config via WiFi"));
	for (int i = 0; i<10; i++)
	{
		if (digitalRead(AP_PIN) == LOW)
		{
			FlashLED(GREEN_LED, 5);
			Serial.println(F("Starting config AP"));
			return true;
		}
		else
		{
			LED(RED_LED, true);
			delay(100);
			LED(RED_LED, false);
			delay(100);
			Serial.print(".");
		}
	}

	Serial.println("");
	return false;
}

void LED(int pPin, bool pOn)
{
	digitalWrite(pPin, (byte)pOn);
}
void FlashLED(int pPin, int pCycles)
{
	for (int i = 0; i < pCycles; i++)
	{
		LED(pPin, true);
		delay(150);
		LED(pPin, false);
		delay(150);
	}
}
void SetupLEDs()
{
	pinMode(GREEN_LED, OUTPUT);
	pinMode(RED_LED, OUTPUT);
}

void SetupPorts()
{
	Serial.println(F("Pin 13 = Electrical meter reading (90ms pulse)"));
	
	gElectricalMeter = new (BUFFER_ELECTRICAL_METER)PulsePort("EL", 13, 1000, 90000, 60, 0);
	gElectricalMeter->ReadTotalValue();
	gElectricalMeter->Begin();

	Serial.println(F("Pin 14 = Heat Pump meter reading (400ms pulse)"));
	gHeatPumpMeter = new (BUFFER_HEAT_PUMP_METER)PulsePort("HP", 14, 1, 400000, 180, 5);
	gHeatPumpMeter->ReadTotalValue();
	gHeatPumpMeter->Begin();
}


String GetConfigPage()
{
	String vOutput = String("");

	unsigned long vMeterValue = -1;
	unsigned long vHPMeterValue = -1;
	unsigned long vTemp;
	if (PulsePort::ReadTotalValue(0, vTemp))
		vMeterValue = vTemp;
	if (PulsePort::ReadTotalValue(5, vTemp))
		vHPMeterValue = vTemp;

	Config vConfig;
	if (vConfig.Load(EEPROM_CONFIG_ADDRESS))
	{
		vOutput += F("<html><body><form method='POST' action='/save'>");
		vOutput += F("<h3>Access Point</h3>");
		vOutput += F("SSID: <input name='ssid' value='");
		vOutput += vConfig.Ssid;
		vOutput += F("'/><br/>");
		vOutput += F("Password: <input name='pwd' type='password' value='");
		vOutput += vConfig.Pwd;
		vOutput += F("'/><br/>");
		vOutput += F("<h3>MQTT</h3>");
		vOutput += F("Server: <input name='msrv' value='");
		vOutput += vConfig.MqttServer;
		vOutput += F("'/><br/>");
		vOutput += F("Topic: <input name='mtop' value='");
		vOutput += vConfig.MqttTopic;
		vOutput += F("'/><br/>");
		vOutput += F("Sensor Name: <input name='mname' value='");
		vOutput += vConfig.MqttName;
		vOutput += F("' /><br/>");
		vOutput += F("<h3>Stored Values</h3>");
		vOutput += F("Electrical: <input name='emeter' value='");
		if (vMeterValue >= 0)
			vOutput += vMeterValue;
		vOutput += F("' /><br/>");
		vOutput += F("Heat Pump: <input name='hpmeter' value='");
		if (vHPMeterValue >= 0)
			vOutput += vHPMeterValue;
		vOutput += F("' /><br/>");
		vOutput += F("<input type='submit' value='submit' />");
		vOutput += F("</body></form></html>");
	}
	else
	{
		vOutput += F("<html><body><form method='POST' action='/save'>SSID: <input name='ssid'/><br/>Password: <input name='pwd' type='password'/><br/><h3>MQTT</h3>Server: <input name='msrv'/><br/>Topic: <input name='mtop'/><br/>Sensor Name: <input name='mname'/><br/><input type='submit' value='submit'/></body></form></html>");
	}

	return vOutput;
}
String HandleSavePage(ESP8266WebServer pServer)
{
	String vSsid = pServer.arg("ssid");
	String vPwd = pServer.arg("pwd");
	String vServer = pServer.arg("msrv");
	String vTopic = pServer.arg("mtop");
	String vName = pServer.arg("mname");


	Config vConfig;
	vConfig.Ssid = new char[vSsid.length() + 1];
	vSsid.toCharArray(vConfig.Ssid, vSsid.length() + 1, 0);
	vConfig.Pwd = new char[vPwd.length() + 1];
	vPwd.toCharArray(vConfig.Pwd, vPwd.length() + 1, 0);
	vConfig.MqttServer = new char[vServer.length() + 1];
	vServer.toCharArray(vConfig.MqttServer, vServer.length() + 1, 0);
	vConfig.MqttTopic = new char[vTopic.length() + 1];
	vTopic.toCharArray(vConfig.MqttTopic, vTopic.length() + 1, 0);
	vConfig.MqttName = new char[vName.length() + 1];
	vName.toCharArray(vConfig.MqttName, vName.length() + 1, 0);
	vConfig.Save(EEPROM_CONFIG_ADDRESS);

	if (pServer.hasArg("emeter") && pServer.hasArg("hpmeter"))
	{
		String vElectrical = pServer.arg("emeter");
		String vHeatPump = pServer.arg("hpmeter");
		if (vElectrical.length() > 0 && vHeatPump.length() > 0)
		{
			unsigned long vElectricalValue = vElectrical.toInt();
			unsigned long vHeatPumpValue = vHeatPump.toInt();
			
			PulsePort::SaveTotalValue(0, vElectricalValue);
			PulsePort::SaveTotalValue(5, vHeatPumpValue);
		}
	}
	
	return String(F("<html><body><h1>Successfully saved configuration</h1><h3>Restart the device...</h3></body></html>"));
}
