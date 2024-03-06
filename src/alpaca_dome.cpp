/*	
  	alpaca_dome.cpp
  	
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

#include <AsyncUDP_ESP32_W5500.hpp>
#include <ESPAsyncWebSrv.h>
#include <esp_task_wdt.h>

#include "defaults.h"
#include "gpio_config.h"
#include "common.h"
#include "alpaca_device.h"
#include "alpaca_server.h"
#include "alpaca_dome.h"
#include "AstroWeatherStation.h"

extern AstroWeatherStation station;

alpaca_dome::alpaca_dome( void ) : alpaca_device( DOME_INTERFACE_VERSION )
{
}
/*
void alpaca_dome::attach_device( Dome &dome )
{
	
}
*/
bool alpaca_dome::abortslew( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( request->method() == HTTP_GET )
		return false;

	if ( get_is_connected() )
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":true,%s})json", transaction_details );
	else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Dome is not connected",%s})json", transaction_details );
	
	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
	return true;
}

bool alpaca_dome::cansetshutter( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( request->method() != HTTP_GET )
		return false;

	if ( get_is_connected() )
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":true,%s})json", transaction_details );
	else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Dome is not connected",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
	return true;
}

bool alpaca_dome::closeshutter( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( request->method() == HTTP_GET )
		return false;
	
	if ( get_is_connected() ) {

		station.get_dome()->close_shutter();
		// dome_shutter_status = dome_shutter_status_t::Closing;

		snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details );
		if ( get_debug_mode() )
			Serial.printf( "[DEBUG] Alpaca dome.closeshutter OK\n" );

	} else {

		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Dome is not connected",%s})json", transaction_details );
		if ( get_debug_mode() )
			Serial.printf( "[DEBUG] Alpaca dome.closeshutter NOK (not connected) : %s\n", message_str );
	}
	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
	return true;
}

bool alpaca_dome::openshutter( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( request->method() == HTTP_GET )
		return false;

	if ( get_is_connected() ) {

		station.get_dome()->open_shutter();
		snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details );

	} else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Dome is not connected",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
	return true;
}

void alpaca_dome::set_connected( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( request->hasParam( "Connected", true ) ) {

		if ( !strcasecmp( request->getParam( "Connected", true )->value().c_str(), "true" )) {

			if ( station.get_dome() != nullptr ) {

				set_is_connected( true );
				snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details );

			} else {
					
				set_is_connected( false );
				snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":1031,"ErrorMessage":"Cannot connect to dome"})json", transaction_details );
					
			}
				
		} else {

			if ( !strcasecmp( request->getParam( "Connected", true )->value().c_str(), "false" )) {

				set_is_connected( false );
				snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details );

			} else

				snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":1025,"ErrorMessage":"Invalid value (%s)"})json", transaction_details, request->getParam( "Connected", true )->value().c_str() );
		}
		request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
		return;
	}

	request->send( 400, "text/plain", "Missing Connected parameter" );
}

void alpaca_dome::slaved( AsyncWebServerRequest *request, const char *transaction_details )
{
	snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":false,%s})json", transaction_details );
	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

bool alpaca_dome::shutterstatus( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( request->method() != HTTP_GET )
		return false;

	if ( get_is_connected() ) {

		// Issue #26 : Of that we can be sure, but the rest?
		if ( station.get_dome()->get_shutter_closed_status() )
			dome_shutter_status = dome_shutter_status_t::Closed;

		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%d,%s})json", static_cast<byte>( dome_shutter_status ), transaction_details );

	} else

		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Dome is not connected",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
	return true;
}
