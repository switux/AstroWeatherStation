/*	
  	alpaca_dome.cpp
  	
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

#include <AsyncUDP_ESP32_W5500.hpp>
#include <ESPAsyncWebSrv.h>

#include "defaults.h"
#include "gpio_config.h"
#include "AstroWeatherStation.h"
#include "alpaca.h"
#include "alpaca_dome.h"
#include "AWS.h"

extern AstroWeatherStation station;

alpaca_dome::alpaca_dome( bool _debug_mode )
{
	is_connected = false;
	debug_mode = _debug_mode;
	devicetype = "Dome";
	dome_shutter_status = dome_shutter_status_t::Error;
	_supportedactions = DOME_SUPPORTED_ACTIONS;
	_driverinfo = DOME_DRIVER_INFO;
	_name = DOME_NAME;
	_description = DOME_DESCRIPTION;
	_driverversion = DOME_DRIVER_VERSION;
	_interfaceversion = DOME_INTERFACE_VERSION;
}

void alpaca_dome::abortslew( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected )
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":true,%s})json", transaction_details );
	else
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1031,"ErrorMessage":"Dome is not connected",%s})json", transaction_details );
	
	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_dome::cansetshutter( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected )
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":true,%s})json", transaction_details );
	else
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1031,"ErrorMessage":"Dome is not connected",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_dome::closeshutter( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected ) {

		station.get_dome()->trigger_close();
		dome_shutter_status = dome_shutter_status_t::Closing;

		snprintf( static_cast<char *>( message_str ), 255, R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details );
		if ( debug_mode )
			Serial.printf( "[DEBUG] Alpaca dome.closeshutter OK\n" );

	} else {

		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1031,"ErrorMessage":"Dome is not connected",%s})json", transaction_details );
		if ( debug_mode )
			Serial.printf( "[DEBUG] Alpaca dome.closeshutter NOK (not connected) : %s\n", message_str );
	}
	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
	
}

void alpaca_dome::openshutter( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected )
		snprintf( static_cast<char *>( message_str ), 255, R"json({%s,"ErrorNumber":1036,"ErrorMessage":"We cannot control dome closure"})json", transaction_details );
	else
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1031,"ErrorMessage":"Dome is not connected",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_dome::set_connected( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( request->hasParam( "Connected", true ) ) {

		if ( !strcasecmp( request->getParam( "Connected", true )->value().c_str(), "true" )) {

			if ( station.get_dome() != NULL ) {

				is_connected = true;
				snprintf( static_cast<char *>( message_str ), 255, R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details );

			} else {
					
				is_connected = false;
				snprintf( static_cast<char *>( message_str ), 255, R"json({%s,"ErrorNumber":1031,"ErrorMessage":"Cannot connect to dome"})json", transaction_details );
					
			}
				
		} else {

			if ( !strcasecmp( request->getParam( "Connected", true )->value().c_str(), "false" )) {

				is_connected = false;
				snprintf( static_cast<char *>( message_str ), 255, R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details );

			} else

				snprintf( static_cast<char *>( message_str ), 255, R"json({%s,"ErrorNumber":1025,"ErrorMessage":"Invalid value (%s)"})json", transaction_details, request->getParam( "Connected", true )->value().c_str() );
		}
		request->send( 200, "application/json", static_cast<const char *>( message_str ) );
		return;
	}

	request->send( 400, "text/plain", "Missing Connected parameter" );
}
/*
void alpaca_dome::slaved( AsyncWebServerRequest *request, const char *transaction_details )
{
	snprintf( static_cast<char *>( message_str ), 255, "{\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":false,%s}", transaction_details );
	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}
*/
void alpaca_dome::shutterstatus( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected ) {

		// FIXME Issue #26 : Of that we can be sure, but the rest?
		if ( station.get_dome()->closed() )
      dome_shutter_status = dome_shutter_status_t::Closed;
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%d,%s})json", static_cast<byte>( dome_shutter_status ), transaction_details );

	} else

		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1031,"ErrorMessage":"Dome is not connected",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}
