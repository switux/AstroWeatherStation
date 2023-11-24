#include <Ethernet.h>
#include <SSLClient.h>
#include <AsyncUDP_ESP32_W5500.hpp>
#include <ESPAsyncWebSrv.h>
#include <TinyGPSPlus.h>
#include <ArduinoJson.h>

#include "defaults.h"
#include "gpio_config.h"
#include "SC16IS750.h"
#include "AWSGPS.h"
#include "AstroWeatherStation.h"
#include "dome.h"
#include "alpaca.h"
#include "alpaca_safetymonitor.h"
#include "SQM.h"
#include "AWSConfig.h"
#include "AWSWeb.h"
#include "AWSSensorManager.h"
#include "AWS.h"

extern AstroWeatherStation station;

alpaca_safetymonitor::alpaca_safetymonitor( bool _debug_mode )
{
	is_connected = false;
	debug_mode = _debug_mode;
	devicetype = "Sensor";
	_supportedactions = SAFETYMONITOR_SUPPORTED_ACTIONS;
	_driverinfo = SAFETYMONITOR_DRIVER_INFO;
	_name = SAFETYMONITOR_NAME;
	_description = SAFETYMONITOR_DESCRIPTION;
	_driverversion = SAFETYMONITOR_DRIVER_VERSION;
	_interfaceversion = SAFETYMONITOR_INTERFACE_VERSION;
}

void alpaca_safetymonitor::issafe( AsyncWebServerRequest *request, const char *transaction_details )
{
	snprintf( (char *)message_str, 255, "{\"Value\":%s,%s}", station.is_rain_event()?"true":"false", transaction_details );
	request->send( 200, "application/json", (const char*)message_str );
}

void alpaca_safetymonitor::set_connected( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( request->hasParam( "Connected", true ) ) {

		if ( !strcasecmp( request->getParam( "Connected", true )->value().c_str(), "true" )) {

			is_connected = true;
			snprintf( (char *)message_str, 255, "{%s,\"ErrorNumber\":0,\"ErrorMessage\":\"\"}", transaction_details );

		} else {

			if ( !strcasecmp( request->getParam( "Connected", true )->value().c_str(), "false" )) {

				is_connected = false;
				snprintf( (char *)message_str, 255, "{%s,\"ErrorNumber\":0,\"ErrorMessage\":\"\"}", transaction_details );

			} else

				snprintf( (char *)message_str, 255, "{%s,\"ErrorNumber\":1025,\"ErrorMessage\":\"Invalid value (%s)\"}", transaction_details, request->getParam( "Connected", true )->value().c_str() );
		}
		request->send( 200, "application/json", (const char*)message_str );
		return;
	}

	request->send( 400, "text/plain", "Missing Connected parameter" );
}
