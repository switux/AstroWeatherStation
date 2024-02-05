/*	
  	alpaca_safetymonitor.cpp
  	
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
#include "common.h"
#include "alpaca_device.h"
#include "alpaca_server.h"
#include "alpaca_safetymonitor.h"
#include "AstroWeatherStation.h"

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
	snprintf( static_cast<char *>( message_str ), 255, "{\"Value\":%s,%s}", station.is_rain_event()?"true":"false", transaction_details );
	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_safetymonitor::set_connected( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( request->hasParam( "Connected", true ) ) {

		if ( !strcasecmp( request->getParam( "Connected", true )->value().c_str(), "true" )) {

			is_connected = true;
			snprintf( static_cast<char *>( message_str ), 255, R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details );

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
