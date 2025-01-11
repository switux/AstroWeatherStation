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

#define DYNAMIC_JSON_DOCUMENT_SIZE  4096	// NOSONAR

#include <Arduino.h>
#include <esp_task_wdt.h>
#include <AsyncTCP.h>
#include <Ethernet.h>
#include <SSLClient.h>
#include <AsyncUDP_ESP32_W5500.hpp>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <LittleFS.h>

#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "defaults.h"
#include "common.h"
#include "config_manager.h"
#include "config_server.h"
#include "AstroWeatherStation.h"

extern HardwareSerial Serial1;					// NOSONAR
extern AstroWeatherStation station;
extern SemaphoreHandle_t sensors_read_mutex;	// Issue #7

void AWSWebServer::attempt_ota_update( AsyncWebServerRequest *request )
{
	station.trigger_ota_update();
	request->send( 200, "text/plain", "Scheduled immediate OTA update" );
	delay( 500 );
}

void AWSWebServer::close_dome_shutter( AsyncWebServerRequest *request )
{
	station.close_dome_shutter();
	request->send( 200, "text/plain", "Processing request" );
	delay( 500 );
}

void AWSWebServer::get_configuration( AsyncWebServerRequest *request )
{
	if ( station.get_json_string_config().size() ) {

		request->send( 200, "application/json", station.get_json_string_config().data() );

	} else

		request->send( 500, "text/plain", "[ERROR] get_configuration() had a problem, please contact support." );
}

void AWSWebServer::get_lookout_rules_state( AsyncWebServerRequest *request )
{
	request->send( 200, "application/json", station.get_lookout_rules_state_json_string().data() );
}

void AWSWebServer::get_station_data( AsyncWebServerRequest *request )
{
	if ( !station.is_ready() ) {

		request->send( 503, "text/plain", "Station is not ready yet" );
		return;
	}

	while ( xSemaphoreTake( sensors_read_mutex, 100 /  portTICK_PERIOD_MS ) != pdTRUE ) { esp_task_wdt_reset(); }
		if ( debug_mode )
			Serial.printf( "[WEBSERVER ] [DEBUG] Waiting for sensor data update to complete.\n" );

	size_t x;
	request->send( 200, "application/json", station.get_json_sensor_data( &x ).data() );
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
	if ( !LittleFS.begin()) {

		etl::string<64> msg;
		Serial.printf( "[WEBSERVER ] [ERROR] Cannot open filesystem to serve index.html." );
		snprintf( msg.data(), msg.capacity(), "[ERROR] Cannot open filesystem to serve index.html" );
		request->send( 500, "text/html", msg.data() );
		return;

	}
	request->send( LittleFS, "/index.html" );
}

bool AWSWebServer::initialise( bool _debug_mode )
{
	int port = station.get_config_port();

	debug_mode = _debug_mode;
	Serial.printf( "[WEBSERVER ] [INFO ] Server on port [%d].\n", port );
	if (( server = new AsyncWebServer(port))) {
			if ( debug_mode )
				Serial.printf( "[WEBSERVER ] [INFO ] AWS Server up.\n" );
	} else {

		Serial.printf( "[WEBSERVER ] [ERROR] Could not start AWS Server.\n" );
		return false;
	}

	initialised = true;
	start();
	return true;
}

void AWSWebServer::open_dome_shutter( AsyncWebServerRequest *request )
{
	station.open_dome_shutter();
	request->send( 200, "text/plain", "Processing request" );
	delay( 500 );
}

void AWSWebServer::resume_lookout( AsyncWebServerRequest *request )
{
	if ( station.resume_lookout() )
		request->send( 200, "text/plain", "OK" );
	else
		request->send( 500, "text/plain", "NOK" );
}

void AWSWebServer::send_file( AsyncWebServerRequest *request )
{
	if ( !LittleFS.begin()) {

		etl::string<64> msg;
		Serial.printf( "[WEBSERVER ] [ERROR] Cannot open filesystem to serve [%s].", request->url().c_str() );
		snprintf( msg.data(), msg.capacity(), "[ERROR] Cannot open filesystem to serve [%s].", request->url().c_str() );
		request->send( 500, "text/html", msg.data() );
		return;

	}
	if ( !LittleFS.exists( request->url().c_str() )) {

		etl::string<64> msg;
		Serial.printf( "[WEBSERVER ] [ERROR] File [%s] not found.", request->url().c_str() );
		snprintf( msg.data(), msg.capacity(), "[ERROR] File [%s] not found.", request->url().c_str() );
		request->send( 500, "text/html", msg.data() );
		return;
	}

	request->send( LittleFS, request->url().c_str() );
	delay(500);
}

void AWSWebServer::reboot( AsyncWebServerRequest *request )
{
	Serial.printf( "[WEBSERVER ] [INFO ] Rebooting...\n" );
	request->send( 200, "text/plain", "OK\n" );
	delay( 500 );
	ESP.restart();
}

void AWSWebServer::rm_file( AsyncWebServerRequest *request )
{
	if ( !LittleFS.begin()) {

		etl::string<64> msg;
		Serial.printf( "[WEBSERVER ] [ERROR] Cannot open filesystem." );
		snprintf( msg.data(), msg.capacity(), "[ERROR] Cannot open filesystem." );
		request->send( 500, "text/html", msg.data() );
		return;
	}
	if ( request->params() !=1 ) {

		etl::string<64> msg;
		Serial.printf( "[WEBSERVER ] [INFO ] rm_file has wrong count of parameter." );
		snprintf( msg.data(), msg.capacity(), "[ERROR] Wrong count of parameter." );
		request->send( 400, "text/html", msg.data() );
		return;
	}

	if ( LittleFS.remove( request->getParam(0)->value() )) {
		request->send( 200, "text/html", "File deleted." );
		return;
	}
	request->send( 500, "text/html", "File not deleted." );
}

void AWSWebServer::set_configuration( AsyncWebServerRequest *request, JsonVariant &json )
{
Serial.printf("SET CONFIG\n");
	if ( station.update_config( json ) ) {

		request->send( 200, "text/plain", "OK\n" );
		station.reboot();

	}
	else
		request->send( 500, "text/plain", "ERROR\n" );
}

void AWSWebServer::start( void )
{
	server->addHandler( new AsyncCallbackJsonWebHandler( "/set_config", std::bind( &AWSWebServer::set_configuration, this, std::placeholders::_1, std::placeholders::_2 )));
	server->on( "/aws.js", HTTP_GET, std::bind( &AWSWebServer::send_file, this, std::placeholders::_1 ));
	server->on( "/close_dome_shutter", HTTP_GET, std::bind( &AWSWebServer::close_dome_shutter, this, std::placeholders::_1 ));
	server->on( "/open_dome_shutter", HTTP_GET, std::bind( &AWSWebServer::open_dome_shutter, this, std::placeholders::_1 ));
	server->on( "/favicon.ico", HTTP_GET, std::bind( &AWSWebServer::send_file, this, std::placeholders::_1 ));
	server->on( "/get_config", HTTP_GET, std::bind( &AWSWebServer::get_configuration, this, std::placeholders::_1 ));
	server->on( "/get_lookout_state", HTTP_GET, std::bind( &AWSWebServer::get_lookout_rules_state, this, std::placeholders::_1 ));
	server->on( "/get_station_data", HTTP_GET, std::bind( &AWSWebServer::get_station_data, this, std::placeholders::_1 ));
	server->on( "/get_root_ca", HTTP_GET, std::bind( &AWSWebServer::get_root_ca, this, std::placeholders::_1 ));
	server->on( "/get_uptime", HTTP_GET, std::bind( &AWSWebServer::get_uptime, this, std::placeholders::_1 ));
	server->on( "/ota_update", HTTP_GET, std::bind( &AWSWebServer::attempt_ota_update, this, std::placeholders::_1 ));
	server->on( "/resume_lookout", HTTP_GET, std::bind( &AWSWebServer::resume_lookout, this, std::placeholders::_1 ));
	server->on( "/suspend_lookout", HTTP_GET, std::bind( &AWSWebServer::suspend_lookout, this, std::placeholders::_1 ));
	server->on( "/rm_file", HTTP_GET, std::bind( &AWSWebServer::rm_file, this, std::placeholders::_1 ));
	server->on( "/", HTTP_GET, std::bind( &AWSWebServer::index, this, std::placeholders::_1 ));
	server->on( "/index.html", HTTP_GET, std::bind( &AWSWebServer::index, this, std::placeholders::_1 ));
	server->on( "/reboot", HTTP_GET, std::bind( &AWSWebServer::reboot, this, std::placeholders::_1 ));
	server->onNotFound( std::bind( &AWSWebServer::handle404, this, std::placeholders::_1 ));
	server->begin();
}

void AWSWebServer::stop( void )
{
	server->reset();
	server->end();
}

void AWSWebServer::suspend_lookout( AsyncWebServerRequest *request )
{
	if ( station.suspend_lookout() )
		request->send( 200, "text/plain", "OK" );
	else
		request->send( 500, "text/plain", "NOK" );
}

void AWSWebServer::handle404( AsyncWebServerRequest *request )
{
	request->send( 404, "text/plain", "Not found\n" );
}
