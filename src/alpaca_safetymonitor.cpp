/*
  	alpaca_safetymonitor.cpp

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

#include "defaults.h"
#include "gpio_config.h"
#include "common.h"
#include "alpaca_device.h"
#include "alpaca_server.h"
#include "alpaca_safetymonitor.h"
#include "AstroWeatherStation.h"

extern AstroWeatherStation station;

alpaca_safetymonitor::alpaca_safetymonitor( void ): alpaca_device( SAFETYMONITOR_INTERFACE_VERSION )
{
}

void alpaca_safetymonitor::issafe( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	snprintf( message_str.data(), message_str.capacity(), "{\"Value\":%s,%s}", station.issafe()?"true":"false", transaction_details.data() );
	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_safetymonitor::set_connected( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( request->hasParam( "Connected", true ) ) {

		if ( !strcasecmp( request->getParam( "Connected", true )->value().c_str(), "true" ) || !strcasecmp( request->getParam( "Connected", true )->value().c_str(), "false" ) ) {

			set_is_connected( true );
			snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details.data() );

		} else

			snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":1025,"ErrorMessage":"Invalid value (%s)"})json", transaction_details.data(), request->getParam( "Connected", true )->value().c_str() );

		request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
		return;

	}
	request->send( 400, "text/plain", "Missing Connected parameter" );
}
