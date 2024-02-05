/*	
  	alpaca_device.cpp
  	
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

#include "alpaca_device.h"

alpaca_device::alpaca_device( void )
{
	memset( message_str, 0, 256 );
}

void alpaca_device::description( AsyncWebServerRequest *request, const char *transaction_details )
{
	snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":"%s",%s})json", _description, transaction_details );
	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_device::driverinfo( AsyncWebServerRequest *request, const char *transaction_details )
{
	snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":"%s",%s})json", _driverinfo, transaction_details );
	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_device::driverversion( AsyncWebServerRequest *request, const char *transaction_details )
{
	snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":"%s",%s})json", _driverversion, transaction_details );
	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_device::get_connected( AsyncWebServerRequest *request, const char *transaction_details )
{
	snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":"%s",%s})json", is_connected?"true":"false", transaction_details );
	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_device::interfaceversion( AsyncWebServerRequest *request, const char *transaction_details )
{
	snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%d,%s})json", _interfaceversion, transaction_details );
	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_device::name( AsyncWebServerRequest *request, const char *transaction_details )
{
	snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":"%s",%s})json", _name, transaction_details );
	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_device::not_implemented( AsyncWebServerRequest *request, const char *transaction_details, const char *msg )
{
	snprintf( static_cast<char *>( message_str ), 255, R"json({%s,"ErrorNumber":1024,"ErrorMessage":"%s"})json", transaction_details, msg?msg:"" );
	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_device::device_error( AsyncWebServerRequest *request, const char *transaction_details, ascom_driver_error error, char *msg )
{
	if ( is_connected )
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":%d,"ErrorMessage":"%s","Value":false,%s})json", 0x500 + static_cast<byte>( error ), msg, transaction_details );
	else
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1031,"ErrorMessage":"%s is not connected",%s})json", devicetype, transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_device::default_bool( AsyncWebServerRequest *request, const char *transaction_details, bool truefalse )
{
	if ( is_connected )
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%s,%s})json", truefalse?"true":"false", transaction_details );
	else
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1031,"ErrorMessage":"%s is not connected",%s})json", devicetype, transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str ) );

}

void alpaca_device::return_value( AsyncWebServerRequest *request, const char *transaction_details, double value )
{
	if ( is_connected )
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%f,%s})json", value, transaction_details );
	else
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1031,"ErrorMessage":"%s is not connected",%s})json", devicetype, transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str ) );

}

void alpaca_device::return_value( AsyncWebServerRequest *request, const char *transaction_details, byte value )
{
	if ( is_connected )
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%d,%s})json", value, transaction_details );
	else
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1031,"ErrorMessage":"%s is not connected",%s})json", devicetype, transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str ) );

}

void alpaca_device::supportedactions( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected )
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%s,%s})json", _supportedactions, transaction_details );
	else
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1031,"ErrorMessage":"%s is not connected",%s})json", devicetype, transaction_details );

	request->send( 200, "application/json", static_cast<const char*>( message_str ));
}
