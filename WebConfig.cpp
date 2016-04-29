// 
// 
// 

#include "WebConfig.h"

Web_Get_Function WebConfig::gConfigPageFunction = 0;
Web_Response_Function WebConfig::gSavePageFunction = 0;
ESP8266WebServer WebConfig::gWebServer = ESP8266WebServer(80);

WebConfig::WebConfig(Web_Get_Function pConfigPageFunction, Web_Response_Function pSavePageFunction)
{
	WebConfig::gConfigPageFunction = pConfigPageFunction;
	WebConfig::gSavePageFunction = pSavePageFunction;
}

void WebConfig::Setup()
{
	WiFi.disconnect(true);
	WiFi.softAPdisconnect(true);
	WiFi.mode(WIFI_OFF);
	delay(2000);

	WiFi.softAP(AP_SSID);
	WiFi.mode(WIFI_AP);

	Serial.print("Configuring access point as ");
	Serial.println(AP_SSID);

	IPAddress vIP = WiFi.softAPIP();
	Serial.print("AP IP Address: ");
	Serial.println(vIP);
	
	SetupWebServer();

	while (true)
		gWebServer.handleClient();
}

void WebConfig::SetupWebServer()
{
	
	Serial.println("Setting up web server on port 80");
	gWebServer.on("/", WebConfig::WebServer_GetConfig);
	gWebServer.on("/save", HTTP_POST, WebConfig::WebServer_SaveConfig);
	gWebServer.begin();
}

void WebConfig::WebServer_GetConfig()
{
	Serial.println("Responding to HTTP request at '/'");
	String vHtmlText = gConfigPageFunction();
	gWebServer.send(200, "text/html", vHtmlText);
}

void WebConfig::WebServer_SaveConfig()
{
	String vHtmlText = gSavePageFunction(gWebServer);
	gWebServer.send(200, "text/html", vHtmlText);
}

const char* WebConfig::AP_SSID = "ESP_Pulse_Meter";
/*
const char* WebConfig::AP_CONFIG_PAGE = "<html><body><form method='POST' action='/save'>SSID: <input name='ssid'/><br/>Password: <input name='pwd' type='password'/><br/><h3>MQTT</h3>Server: <input name='msrv'/><br/>Topic: <input name='mtop'/><br/>Sensor Name: <input name='mname'/><br/><input type='submit' value='submit'/></body></form>";
const char* WebConfig::AP_SUCCESS_PAGE = "<html><body><h1>Successflly Saved!</h1><h3>Please restart the device now</h3></body></form>";
*/

void WebConfig::Handle() 
{
	WebConfig::gWebServer.handleClient();
}