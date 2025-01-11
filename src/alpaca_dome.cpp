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
#include <ESPAsyncWebServer.h>
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

bool alpaca_dome::abortslew( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( request->method() == HTTP_GET )
		return false;

	if ( get_is_connected() )
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":true,%s})json", transaction_details.data() );
	else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Dome is not connected",%s})json", transaction_details.data() );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
	return true;
}

bool alpaca_dome::cansetshutter( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( request->method() != HTTP_GET )
		return false;

	if ( get_is_connected() )
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":true,%s})json", transaction_details.data() );
	else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Dome is not connected",%s})json", transaction_details.data() );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
	return true;
}

bool alpaca_dome::closeshutter( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( request->method() == HTTP_GET )
		return false;

	if ( get_is_connected() ) {

		station.get_dome()->close_shutter();

		snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details.data() );
		if ( get_debug_mode() )
			Serial.printf( "[ALPACADOME] [DEBUG] dome.closeshutter OK\n" );

	} else {

		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Dome is not connected",%s})json", transaction_details.data() );
		if ( get_debug_mode() )
			Serial.printf( "[ALPACADOME] [DEBUG] dome.closeshutter NOK (not connected) : %s\n", message_str );
	}
	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
	return true;
}

bool alpaca_dome::openshutter( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( request->method() == HTTP_GET )
		return false;

	if ( get_is_connected() ) {

		station.get_dome()->open_shutter();
		snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details.data() );

	} else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Dome is not connected",%s})json", transaction_details.data() );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
	return true;
}

void alpaca_dome::set_connected( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( request->hasParam( "Connected", true ) ) {

		if ( !strcasecmp( request->getParam( "Connected", true )->value().c_str(), "true" )) {

			if ( station.get_dome() != nullptr ) {

				set_is_connected( true );
				snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details.data() );

			} else {

				set_is_connected( false );
				snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":1031,"ErrorMessage":"Cannot connect to dome"})json", transaction_details.data() );

			}

		} else {

			if ( !strcasecmp( request->getParam( "Connected", true )->value().c_str(), "false" )) {

				set_is_connected( false );
				snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details.data() );

			} else

				snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":1025,"ErrorMessage":"Invalid value (%s)"})json", transaction_details.data(), request->getParam( "Connected", true )->value().c_str() );
		}
		request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
		return;
	}

	request->send( 400, "text/plain", "Missing Connected parameter" );
}

void alpaca_dome::slaved( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":false,%s})json", transaction_details.data() );
	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

bool alpaca_dome::shutterstatus( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( request->method() != HTTP_GET )
		return false;

	if ( get_is_connected() ) {

		// Issue #26 : Of that we can be sure, but the rest?
		if ( station.get_dome()->get_shutter_closed_status() )
			dome_shutter_status = dome_shutter_status_t::Closed;
		if ( station.get_dome()->get_shutter_open_status() )
			dome_shutter_status = dome_shutter_status_t::Open;

		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%d,%s})json", static_cast<byte>( dome_shutter_status ), transaction_details.data() );

	} else

		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Dome is not connected",%s})json", transaction_details.data() );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
	return true;
}
