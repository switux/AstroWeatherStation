/*
	AWSAlpaca.cpp

	ASCOM ALPACA Server for the AstroWeatherStation (c) 2023 F.Lesage

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
#include <AsyncTCP.h>
#include <Ethernet.h>
#include <SSLClient.h>
#include <AsyncUDP_ESP32_W5500.hpp>
#include <ESPAsyncWebSrv.h>
#include <SPIFFS.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>


#include "AWSGPS.h"
#include "AstroWeatherStation.h"
#include "SQM.h"
#include "SC16IS750.h"
#include "AWSSensorManager.h"
#include "AWSWeb.h"
#include "alpaca.h"
#include "AWS.h"


extern AstroWeatherStation station;

alpaca_server::alpaca_server( void )
{
}

bool alpaca_server::get_debug_mode( void )
{
	return debug_mode;
}

void alpaca_server::on_packet( AsyncUDPPacket packet )
{
	if ( packet.length() ) {

		int len = packet.read( (uint8_t*)buf, 255 );
		if ( len > 0 )
			buf[ len ] = 0;
		if ( len < 16 ) {
			if ( debug_mode )
				Serial.printf( "[INFO] ALPACA Discovery: short packet [%s] from %s:%d\n" , buf, packet.remoteIP().toString().c_str(), packet.remotePort() );
			return;
		}
		if ( strncmp( "alpacadiscovery1", buf, 16 ) != 0 ) {
			if ( debug_mode )
				Serial.printf( "[INFO] ALPACA Discovery: bad header [%s] from %s:%d\n",  buf, packet.remoteIP().toString().c_str(), packet.remotePort() );
			return;
		}
		sprintf( buf, "{\"AlpacaPort\":%d}", ALPACA_SERVER_PORT );
		ascom_discovery.writeTo( (uint8_t *)buf, strlen( buf ), packet.remoteIP(), packet.remotePort(), packet.interface() );
	}
}

bool alpaca_server::start( IPAddress address, bool _debug_mode )
{
	debug_mode = _debug_mode;
	
	if ( !server_up ) {
		server = new AsyncWebServer( ALPACA_SERVER_PORT );
		if (server)
			Serial.printf("ALPACA: started server\n" );
		else
			Serial.printf("ALPACA: NOT started server\n" );

	}
	if ( !ascom_discovery.listen( 32227 ))
	
	if ( debug_mode )
		Serial.printf( "[INFO] ALPACA Discovery server started on %s:%d\n", address.toString().c_str(), 32227 );
	
	ascom_discovery.onPacket( std::bind( &alpaca_server::on_packet, this, std::placeholders::_1 ));
	
	server->on( "/get_config", HTTP_GET, std::bind( &alpaca_server::get_config, this, std::placeholders::_1 ));
	
	server->on( "/setup", HTTP_GET, std::bind( &alpaca_server::alpaca_getsetup, this, std::placeholders::_1 ));
	server->on( "/management/apiversions", HTTP_GET, std::bind( &alpaca_server::alpaca_getapiversions, this, std::placeholders::_1 ));
	server->on( "/management/v1/description", HTTP_GET, std::bind( &alpaca_server::alpaca_getdescription, this, std::placeholders::_1 ));
	server->on( "/management/v1/configureddevices", HTTP_GET, std::bind( &alpaca_server::alpaca_getconfigureddevices, this, std::placeholders::_1 ));
	server->on( "/favicon.ico", HTTP_GET, std::bind( &alpaca_server::send_file, this, std::placeholders::_1 ));

	server->onNotFound( std::bind( &alpaca_server::handle404, this, std::placeholders::_1 ));
	server->begin();

	return true;
}

void alpaca_server::get_config( AsyncWebServerRequest *request )
{
	char *json_string = station.get_json_string_config();

	if ( json_string ) {

		request->send( 200, "application/json", json_string );
		free( json_string );

	} else

		request->send( 500, "text/plain", "[ERROR] get_configuration() had a problem, please contact support." );
}

alpaca_server *alpaca_server::get_alpaca( void )
{
	return this;
}

AsyncUDP *alpaca_server::get_discovery( void )
{
	return &ascom_discovery;
}


void alpaca_server::handle404( AsyncWebServerRequest *request )
{
	Serial.printf("ALPACA NOT FOUND: %s\n", request->url().c_str());
}

void alpaca_server::alpaca_getsetup( AsyncWebServerRequest *request )
{
	request->send( SPIFFS, "/ascom_home.html" );
}

const char *alpaca_server::get_transaction_details( AsyncWebServerRequest *request )
{
	for( int i = 0; i < request->params(); i++ ) {
		Serial.printf("ARG %d:%s=%ld",i,request->getParam(i)->name().c_str(),request->getParam(i)->value().toInt());
	}
	static char str[128];
	sprintf( str, "\"ClientID\":%d,\"ClientTransactionID\":%d,\"ServerTransactionID\":%d,\"ErrorNumber\":0,\"ErrorMessage\":\"\"", client_id, client_transaction_id, server_transaction_id );
	return str;
}

void alpaca_server::alpaca_getapiversions( AsyncWebServerRequest *request )
{
	server_transaction_id++;
	unsigned char str[128];
	sprintf( (char *)str, "{\"Value\":[1],%s}", get_transaction_details( request ) );
	request->send( 200, "application/json", (const char*)str );
}

void alpaca_server::alpaca_getdescription( AsyncWebServerRequest *request )
{
	server_transaction_id++;
	unsigned char str[256];
	sprintf( (char *)str, "{\"Value\":%s,%s}", "{\"ServerName\":\"AWS\",\"Manufacturer\":\"bibi\",\"ManufacturerVersion\":\"v2.1.x\",\"Location\":\"ici\"}", "\"ClientTransactionID\":0,\"ServerTransactionID\":0" );
	request->send( 200, "application/json", (const char*)str );
}

void alpaca_server::alpaca_getconfigureddevices( AsyncWebServerRequest *request )
{
	server_transaction_id++;
	unsigned char str[256];
	sprintf( (char *)str, "{\"Value\":[{\"DeviceName\":\"AstroWeatherstation\",\"DeviceType\":\"ObservingConditions\",\"DeviceNumber\":0,\"UniqueID\":\"%s\"}],%s}", AWS_UUID, "\"ClientTransactionID\":0,\"ServerTransactionID\":0" );
	request->send( 200, "application/json", (const char*)str );
	
}

void alpaca_server::send_file( AsyncWebServerRequest *request )
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

void alpaca_server::stop( void )
{
	server->end();
}

void alpaca_server::stop_discovery( void )
{
	ascom_discovery.close();
}
