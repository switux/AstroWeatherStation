/*
  	config_server.cpp

	(c) 2023-2024 F.Lesage

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

#include <Arduino.h>
#include <esp_task_wdt.h>
#include <AsyncTCP.h>
#include <Ethernet.h>
#include <SSLClient.h>
#include <AsyncUDP_ESP32_W5500.hpp>
#include <ESPAsyncWebSrv.h>
#include <FS.h>
#include <SPIFFS.h>

#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "defaults.h"
#include "common.h"
#include "config_manager.h"
#include "config_server.h"
#include "AstroWeatherStation.h"

extern HardwareSerial Serial1;
extern AstroWeatherStation station;
extern SemaphoreHandle_t sensors_read_mutex;	// FIXME: hide this within the sensor manager

void AWSWebServer::attempt_ota_update( AsyncWebServerRequest *request )
{
	request->send( 200, "text/plain", "Processing request" );
	station.check_ota_updates();
}

void AWSWebServer::get_configuration( AsyncWebServerRequest *request )
{
	if ( station.get_json_string_config().size() ) {

		request->send( 200, "application/json", station.get_json_string_config().data() );

	} else

		request->send( 500, "text/plain", "[ERROR] get_configuration() had a problem, please contact support." );
}

void AWSWebServer::get_station_data( AsyncWebServerRequest *request )
{
	if ( !station.is_ready() ) {

		request->send( 503, "text/plain", "Station is not ready yet" );
		return;
	}
	
	while ( xSemaphoreTake( sensors_read_mutex, 100 /  portTICK_PERIOD_MS ) != pdTRUE ) { esp_task_wdt_reset(); }
		if ( debug_mode )
			Serial.printf( "[DEBUG] Waiting for sensor data update to complete.\n" );

	request->send( 200, "application/json", station.get_json_sensor_data().data() );
	xSemaphoreGive( sensors_read_mutex );
}

void AWSWebServer::get_root_ca( AsyncWebServerRequest *request )
{
	request->send( 200, "text/plain", station.get_root_ca().data() );
}

void AWSWebServer::get_uptime( AsyncWebServerRequest *request )
{
	int32_t			uptime	= station.get_uptime();
	int				days	= floor( uptime / ( 3600 * 24 ));
	int				hours	= floor( fmod( uptime, 3600 * 24 ) / 3600 );
	int				minutes	= floor( fmod( uptime, 3600 ) / 60 );
	int				seconds	= fmod( uptime, 60 );
	etl::string<16>	str;
	
	snprintf( str.data(), str.capacity(), "%03dd:%02dh:%02dm:%02ds", days, hours, minutes, seconds );
	request->send( 200, "text/plain", str.data() );
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
	server->on( "/get_station_data", HTTP_GET, std::bind( &AWSWebServer::get_station_data, this, std::placeholders::_1 ));
	server->on( "/get_root_ca", HTTP_GET, std::bind( &AWSWebServer::get_root_ca, this, std::placeholders::_1 ));
	server->on( "/get_uptime", HTTP_GET, std::bind( &AWSWebServer::get_uptime, this, std::placeholders::_1 ));
	server->on( "/ota_update", HTTP_GET, std::bind( &AWSWebServer::attempt_ota_update, this, std::placeholders::_1 ));
	server->on( "/", HTTP_GET, std::bind( &AWSWebServer::index, this, std::placeholders::_1 ));
	server->on( "/index.html", HTTP_GET, std::bind( &AWSWebServer::index, this, std::placeholders::_1 ));
	server->on( "/reboot", HTTP_GET, std::bind( &AWSWebServer::reboot, this, std::placeholders::_1 ));
	server->onNotFound( std::bind( &AWSWebServer::handle404, this, std::placeholders::_1 ));
	server->begin();
	return true;
}

void AWSWebServer::send_file( AsyncWebServerRequest *request )
{
	if ( !SPIFFS.exists( request->url().c_str() )) {

		etl::string<64> msg;
		Serial.printf( "[ERROR] File [%s] not found.", request->url().c_str() );
		snprintf( msg.data(), msg.capacity(), "[ERROR] File [%s] not found.", request->url().c_str() );
		request->send( 500, "text/html", msg.data() );
		return;
	}

	request->send( SPIFFS, request->url().c_str() );
	delay(500);
}

void AWSWebServer::reboot( AsyncWebServerRequest *request )
{
	Serial.printf( "Rebooting...\n" );
	request->send( 200, "text/plain", "OK\n" );
	delay( 500 );
	ESP.restart();
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
