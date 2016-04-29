// WebConfig.h

#ifndef _WEBCONFIG_h
#define _WEBCONFIG_h

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif


typedef String(*Web_Get_Function)();
typedef String(*Web_Response_Function)(ESP8266WebServer pWebServer);

class WebConfig
{
public:
	WebConfig(Web_Get_Function pConfigPageFunction, Web_Response_Function pSavePageFunction);
	void Setup();
	void Handle();

protected:
	static void SetupWebServer();
	static void WebServer_GetConfig();
	static void WebServer_SaveConfig();

private:
	static Web_Get_Function gConfigPageFunction;
	static Web_Response_Function gSavePageFunction;
	static const char* AP_SSID;
	static const char* AP_CONFIG_PAGE;
	static const char* AP_SUCCESS_PAGE;
	static ESP8266WebServer gWebServer;

};

#endif
