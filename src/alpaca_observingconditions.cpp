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
#include "alpaca_observingconditions.h"
#include "SQM.h"
#include "AWSConfig.h"
#include "AWSWeb.h"
#include "AWSSensorManager.h"
#include "AWS.h"

extern AstroWeatherStation station;

alpaca_observingconditions::alpaca_observingconditions( bool _debug_mode )
{
	is_connected = false;
	debug_mode = _debug_mode;
	devicetype = "Sensor";
	_supportedactions = OBSERVINGCONDITIONS_SUPPORTED_ACTIONS;
	_driverinfo = OBSERVINGCONDITIONS_DRIVER_INFO;
	_name = OBSERVINGCONDITIONS_NAME;
	_description = OBSERVINGCONDITIONS_DESCRIPTION;
	_driverversion = OBSERVINGCONDITIONS_DRIVER_VERSION;
	_interfaceversion = OBSERVINGCONDITIONS_INTERFACE_VERSION;
}

void alpaca_observingconditions::set_connected( AsyncWebServerRequest *request, const char *transaction_details )
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

				snprintf( (char *)message_str, 255, "{%s,\"ErrorNumber\":1025,\"ErrorMessage\":\"Invalid value %s\"}", transaction_details, request->getParam( "Connected", true )->value().c_str() );
		}
		request->send( 200, "application/json", (const char*)message_str );
		return;
	}

	request->send( 400, "text/plain", "Missing Connected parameter" );
}

void alpaca_observingconditions::get_averageperiod( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected )
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":0.0,%s}", transaction_details );
	else
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":1031,\"ErrorMessage\":\"Sensor is not connected\",%s}", transaction_details );

	request->send( 200, "application/json", (const char*)message_str );
}


void alpaca_observingconditions::set_averageperiod( AsyncWebServerRequest *request, const char *transaction_details )
{
	float	x;
	
	if ( is_connected ) {
			if ( request->hasParam( "AveragePeriod", true ) ) {

				x = atof( request->getParam( "AveragePeriod", true )->value().c_str() );
				if ( x == 0 )

					snprintf( (char *)message_str, 255, "{\"ErrorNumber\":0,\"ErrorMessage\":\"\",%s}", transaction_details );

				else {

					if ( x < 0 )
						snprintf( (char *)message_str, 255, "{\"ErrorNumber\":%d,\"ErrorMessage\":\"Value must be positive or 0\",%s}", 1023+InvalidValue, transaction_details );
					else
						snprintf( (char *)message_str, 255, "{\"ErrorNumber\":%d,\"ErrorMessage\":\"Only providing live data, please set to 0.\",%s}", 1023+InvalidValue, transaction_details );
				}				
			} else {
				
				request->send( 400, "text/plain", "Missing Connected parameter" );
				return;
			}
	}
	else
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":1031,\"ErrorMessage\":\"Sensor is not connected\",%s}", transaction_details );

	request->send( 200, "application/json", (const char*)message_str );
}

void alpaca_observingconditions::cloudcover( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected )
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":%3.1f,%s}", station.get_sensor_data()->cloud_cover, transaction_details );
	else
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":1031,\"ErrorMessage\":\"Sensor is not connected\",%s}", transaction_details );

	request->send( 200, "application/json", (const char*)message_str );
}

void alpaca_observingconditions::dewpoint( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected )
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":%2.1f,%s}", station.get_sensor_data()->dew_point, transaction_details );
	else
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":1031,\"ErrorMessage\":\"Sensor is not connected\",%s}", transaction_details );

	request->send( 200, "application/json", (const char*)message_str );
}

void alpaca_observingconditions::humidity( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected )
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":%3.1f,%s}", station.get_sensor_data()->rh, transaction_details );
	else
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":1031,\"ErrorMessage\":\"Sensor is not connected\",%s}", transaction_details );

	request->send( 200, "application/json", (const char*)message_str );
}

void alpaca_observingconditions::pressure( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected )
		sprintf( (char *)message_str, "{\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":%4.1f,%s}", station.get_sensor_data()->pressure, transaction_details );
	else
		sprintf( (char *)message_str, "{\"ErrorNumber\":1031,\"ErrorMessage\":\"Sensor is not connected\",%s}", transaction_details );

	request->send( 200, "application/json", (const char*)message_str );
}

void alpaca_observingconditions::rainrate( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( !station.has_rain_sensor() )

		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":%d,\"ErrorMessage\":\"The station has no rain sensor\",%s}", PropertyOrMethodNotImplemented, transaction_details );

	else {

		if ( is_connected ) {

			short x = station.get_sensor_data()->rain_intensity;

			if ( x >= 0 )
				snprintf( (char *)message_str, 255, "{\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":%3.1f,%s}", rain_rate[ x ], transaction_details );
			else
				snprintf( (char *)message_str, 255, "{\"ErrorNumber\":%d,\"ErrorMessage\":\"Rain sensor data is temporarily unavailable\",%s}", PropertyOrMethodNotImplemented, transaction_details );
		
		} else

			snprintf( (char *)message_str, 255, "{\"ErrorNumber\":1031,\"ErrorMessage\":\"Sensor is not connected\",%s}", transaction_details );
	}

	request->send( 200, "application/json", (const char*)message_str );
}

void alpaca_observingconditions::skybrightness( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected )
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":%6.4f,%s}", (double)station.get_sensor_data()->lux, transaction_details );
	else
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":1031,\"ErrorMessage\":\"Sensor is not connected\",%s}", transaction_details );

	request->send( 200, "application/json", (const char*)message_str );
}

void alpaca_observingconditions::skyquality( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected )
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":%2.2f,%s}", station.get_sensor_data()->msas, transaction_details );
	else
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":1031,\"ErrorMessage\":\"Sensor is not connected\",%s}", transaction_details );

	request->send( 200, "application/json", (const char*)message_str );
}

void alpaca_observingconditions::skytemperature( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected )
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":%2.2f,%s}", station.get_sensor_data()->sky_temperature, transaction_details );
	else
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":1031,\"ErrorMessage\":\"Sensor is not connected\",%s}", transaction_details );

	request->send( 200, "application/json", (const char*)message_str );
}

void alpaca_observingconditions::temperature( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected )
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":%2.2f,%s}", station.get_sensor_data()->temperature, transaction_details );
	else
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":1031,\"ErrorMessage\":\"Sensor is not connected\",%s}", transaction_details );

	request->send( 200, "application/json", (const char*)message_str );
}

void alpaca_observingconditions::winddirection( AsyncWebServerRequest *request, const char *transaction_details )
{
	uint16_t x = station.get_sensor_data()->wind_direction;

	if ( station.get_sensor_data()->wind_speed > 0 )
		x = ( x == 0 ) ? 360 : x;
	else
		x = 0;

	if ( is_connected )
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":%3.1f,%s}", (double)x, transaction_details );
	else
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":1031,\"ErrorMessage\":\"Sensor is not connected\",%s}", transaction_details );

	request->send( 200, "application/json", (const char*)message_str );
}

void alpaca_observingconditions::windgust( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected )
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":%3.1f,%s}", station.get_sensor_data()->wind_gust, transaction_details );
	else
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":1031,\"ErrorMessage\":\"Sensor is not connected\",%s}", transaction_details );

	request->send( 200, "application/json", (const char*)message_str );
}

void alpaca_observingconditions::windspeed( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected )
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":%3.1f,%s}", station.get_sensor_data()->wind_speed, transaction_details );
	else
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":1031,\"ErrorMessage\":\"Sensor is not connected\",%s}", transaction_details );

	request->send( 200, "application/json", (const char*)message_str );
}

void alpaca_observingconditions::refresh( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected ) {

		if ( station.poll_sensors() )

			snprintf( (char *)message_str, 255, "{\"ErrorNumber\":0,\"ErrorMessage\":\"\",%s}", transaction_details );

		else {

			request->send( 400, "application/json", "Could not refresh sensors" );
			return;
			
		}
	} else

		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":1031,\"ErrorMessage\":\"Sensor is not connected\",%s}", transaction_details );

	request->send( 200, "application/json", (const char*)message_str );
}

void alpaca_observingconditions::sensordescription( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected )
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":true,%s}", transaction_details );
	else
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":1031,\"ErrorMessage\":\"Dome is not connected\",%s}", transaction_details );

	request->send( 200, "application/json", (const char*)message_str );
}

void alpaca_observingconditions::timesincelastupdate( AsyncWebServerRequest *request, const char *transaction_details )
{
	time_t now;
	time( &now );
	
	if ( is_connected )
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":%3.1f,%s}", (double)( now - station.get_sensor_data()->timestamp ), transaction_details );
	else
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":1031,\"ErrorMessage\":\"Dome is not connected\",%s}", transaction_details );

	request->send( 200, "application/json", (const char*)message_str );
}