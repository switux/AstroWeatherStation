/*	
  	AWSWeb.cpp
  	
	(c) 2023 F.Lesage

	This program is free software: you can redistribute it and/or modify it
	under the terms of the GNU General Public License as published by the
	Free Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful, but
	WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
	or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
	more details.

	You should have received a copy of the GNU General Public License along
	with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#undef CONFIG_DISABLE_HAL_LOCKS
#define _ASYNC_WEBSERVER_LOGLEVEL_       0
#define _ETHERNET_WEBSERVER_LOGLEVEL_      0
#define ASYNCWEBSERVER_REGEX	1

#include <Arduino.h>
#include <AsyncTCP.h>
#include <Ethernet.h>
#include <SSLClient.h>
#include <AsyncUDP_ESP32_W5500.hpp>
#include <ESPAsyncWebSrv.h>
#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>

#include "time.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "FS.h"
#include "SPIFFS.h"
#include "SC16IS750.h"
#include "AWSGPS.h"
#include "AstroWeatherStation.h"
#include "SQM.h"
#include "AWSConfig.h"
#include "AWSWeb.h"
#include "SC16IS750.h"
#include "AWSSensorManager.h"
#include "dome.h"
#include "alpaca.h"
#include "AWS.h"


extern AstroWeatherStation station;
extern SemaphoreHandle_t sensors_read_mutex;	// FIXME: hide this within the sensor manager

AWSWebServer::AWSWebServer( void )
{
}

AWSWebServer::~AWSWebServer( void )
{
	delete server;
}

void AWSWebServer::get_configuration( AsyncWebServerRequest *request )
{
	char *json_string = station.get_json_string_config();

	if ( json_string ) {

		request->send( 200, "application/json", json_string );
		free( json_string );

	} else

		request->send( 500, "text/plain", "[ERROR] get_configuration() had a problem, please contact support." );
}

void AWSWebServer::get_data( AsyncWebServerRequest *request )
{
	while ( xSemaphoreTake( sensors_read_mutex, 100 /  portTICK_PERIOD_MS ) != pdTRUE )
		if ( debug_mode )
			Serial.printf( "[DEBUG] Waiting for sensor data update to complete.\n" );

	char *json_data = station.get_json_sensor_data();
	request->send( 200, "application/json", json_data );
	xSemaphoreGive( sensors_read_mutex );
}

void AWSWebServer::get_root_ca( AsyncWebServerRequest *request )
{
	request->send( 200, "text/plain", station.get_root_ca() );
}

void AWSWebServer::get_uptime( AsyncWebServerRequest *request )
{
	request->send( 200, "text/plain", station.get_uptime() );
}

void AWSWebServer::index( AsyncWebServerRequest *request )
{
	request->send( SPIFFS, "/index.html" );
	delay( 500 );		// FIXME: why ...?
}

bool AWSWebServer::initialise( bool _debug_mode )
{
	int port = station.get_config_port();

	debug_mode = _debug_mode;
	Serial.printf( "[INFO] Server on port [%d].\n", port );
	if (( server = new AsyncWebServer(port))) {
			if ( debug_mode )
				Serial.printf( "[INFO] AWS Server up.\n" );
	} else {

		Serial.printf( "[ERROR] Could not start AWS Server.\n" );
		return false;
	}

	server->addHandler( new AsyncCallbackJsonWebHandler( "/set_config", std::bind( &AWSWebServer::set_configuration, this, std::placeholders::_1, std::placeholders::_2 )));
	server->on( "/favicon.ico", HTTP_GET, std::bind( &AWSWebServer::send_file, this, std::placeholders::_1 ));
	server->on( "/configuration.js", HTTP_GET, std::bind( &AWSWebServer::send_file, this, std::placeholders::_1 ));
	server->on( "/get_config", HTTP_GET, std::bind( &AWSWebServer::get_configuration, this, std::placeholders::_1 ));
	server->on( "/get_data", HTTP_GET, std::bind( &AWSWebServer::get_data, this, std::placeholders::_1 ));
	server->on( "/get_root_ca", HTTP_GET, std::bind( &AWSWebServer::get_root_ca, this, std::placeholders::_1 ));
	server->on( "/get_uptime", HTTP_GET, std::bind( &AWSWebServer::get_uptime, this, std::placeholders::_1 ));
	server->on( "/", HTTP_GET, std::bind( &AWSWebServer::index, this, std::placeholders::_1 ));
	server->on( "/index.html", HTTP_GET, std::bind( &AWSWebServer::index, this, std::placeholders::_1 ));
	server->on( "/reboot", HTTP_GET, std::bind( &AWSWebServer::reboot, this, std::placeholders::_1 ));
	server->onNotFound( std::bind( &AWSWebServer::handle404, this, std::placeholders::_1 ));
	server->begin();		
	return true;
}

void AWSWebServer::send_file( AsyncWebServerRequest *request )
{
	char *filename = strdup( request->url().c_str() );
	char msg[64];
	
	if ( !SPIFFS.exists( filename )) {

		Serial.printf( "[ERROR] File [%s] not found.", filename );
		snprintf( msg, 64, "[ERROR] File [%s] not found.", filename );
		request->send( 500, "text/html", msg );
		free( filename );
		return;
	}

	request->send( SPIFFS, filename );
	delay(500);
	free( filename );
}



void AWSWebServer::reboot( AsyncWebServerRequest *request )
{
	Serial.printf( "Rebooting...\n" );
	request->send( 200, "text/plain", "OK\n" );
	delay( 500 );
	ESP.restart();	
}

void AWSWebServer::reset_config_parameter( AsyncWebServerRequest *request )
{
	//char		parameter[32];

	//snprintf( parameter, 32, server->arg( "plain" ).c_str() );
/*
	if ( config.contains_parameter( parameter )) {

		config.reset_parameter( parameter );
		if ( config.save_runtime_configuration() ) {
			server->send( 200, "text/plain", "OK\n" );
			delay( 1000 );
			return;
		}
		server->send( 500, "text/plain", "Cannot reset parameter." );
	} else {
		server->send( 400, "text/plain", "Parameter not found." );
		return;
	}
	*/
}

void AWSWebServer::set_configuration( AsyncWebServerRequest *request, JsonVariant &json )
{
	if ( station.update_config( json ) ) {

		request->send( 200, "text/plain", "OK\n" );
		station.reboot();
	}
	else
		request->send( 500, "text/plain", "ERROR\n" );
}


void AWSWebServer::handle404( AsyncWebServerRequest *request )
{
	request->send( 404, "text/plain", "Not found\n" );
}
