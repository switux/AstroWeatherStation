/*	
  	alpaca_device.cpp
  	
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

#include "device.h"
#include "alpaca_device.h"

alpaca_device::alpaca_device( alpaca_interface_version_t intver ) : interfaceversion( intver )
{
}
/*
void alpaca_device::attach_device( std::shared_device<Device> _device )
{
	device = _device;
}
*/

bool alpaca_device::get_debug_mode( void )
{
	return debug_mode;
}

bool alpaca_device::get_is_connected( void )
{
	return is_connected;
}

void alpaca_device::send_connected( AsyncWebServerRequest *request, const char *transaction_details )
{
	etl::string<256>	message_str;
	snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":"%s",%s})json", is_connected?"true":"false", transaction_details );
	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_device::not_implemented( AsyncWebServerRequest *request, const char *transaction_details, const char *msg )
{
	etl::string<256>	message_str;
	snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":1024,"ErrorMessage":"%s"})json", transaction_details, msg?msg:"" );
	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_device::device_error( AsyncWebServerRequest *request, const char *transaction_details, ascom_driver_error error, char *msg )
{
	etl::string<256>	message_str;
	if ( is_connected )
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":%d,"ErrorMessage":"%s","Value":false,%s})json", 0x500 + static_cast<byte>( error ), msg, transaction_details );
	else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Device is not connected",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_device::send_value( AsyncWebServerRequest *request, const char *transaction_details, bool truefalse )
{
	etl::string<256>	message_str;
	if ( is_connected )
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%s,%s})json", truefalse?"true":"false", transaction_details );
	else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Device is not connected",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );

}

void alpaca_device::send_value( AsyncWebServerRequest *request, const char *transaction_details, float value )
{
	etl::string<256>	message_str;
	if ( is_connected )
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%f,%s})json", value, transaction_details );
	else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Device is not connected",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );

}

void alpaca_device::send_value( AsyncWebServerRequest *request, const char *transaction_details, byte value )
{
	etl::string<256>	message_str;
	if ( is_connected )
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%d,%s})json", value, transaction_details );
	else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Device is not connected",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );

}

bool alpaca_device::send_description( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( request->method() != HTTP_GET )
		return false;

	etl::string<256>	message_str;
	snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":"%s",%s})json", description, transaction_details );
	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
	return true;
}

bool alpaca_device::send_driverinfo( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( request->method() != HTTP_GET )
		return false;

	etl::string<256>	message_str;
	snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":"%s",%s})json", driverinfo, transaction_details );
	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
	return true;
}

bool alpaca_device::send_driverversion( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( request->method() != HTTP_GET )
		return false;

	etl::string<256>	message_str;
	snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":"%s",%s})json", driverversion, transaction_details );
	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
	return true;
}

bool alpaca_device::send_interfaceversion( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( request->method() != HTTP_GET )
		return false;

	etl::string<256>	message_str;
	snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%d,%s})json", interfaceversion, transaction_details );
	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
	return true;
}

bool alpaca_device::send_name( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( request->method() != HTTP_GET )
		return false;

	etl::string<256>	message_str;
	snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":"%s",%s})json", name, transaction_details );
	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
	return true;
}

bool alpaca_device::send_supportedactions( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( request->method() != HTTP_GET )
		return false;

	etl::string<256>	message_str;
	if ( is_connected )
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%s,%s})json", supportedactions, transaction_details );
	else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Device is not connected",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char*>( message_str.data() ));
	return true;
}

void alpaca_device::set_debug_mode( bool b )
{
	debug_mode = b;
}

void alpaca_device::set_is_connected( bool b )
{
	is_connected = b;
}
