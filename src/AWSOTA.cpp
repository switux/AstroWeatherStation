/*
  	AWSOTA.cpp

	(c) 2024 F.Lesage and 


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

#include <esp_task_wdt.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFi.h>

#include "Embedded_Template_Library.h"
#include "etl/string.h"

#include "AWSOTA.h"

AWSOTA::AWSOTA( void )
{
	json_ota_config = new DynamicJsonDocument( 6000 );
}

int AWSOTA::check_for_update( const char *url, etl::string<26> &current_version, ActionType action = DONT_DO_UPDATE )
{
	etl::string<32>	board;
	etl::string<32>	config;
	int				deserialisation_status;
	etl::string<32>	device;
	bool			found_profile = false;
	int				http_status;
	etl::string<32>	version;

	http_status = download_json( url );
	if ( http_status != 200 )
		return http_status > 0 ? http_status : HTTP_FAILED;

	if ( !board_name.size() || !config_name.size() || !device_name.size() )
		return CONFIG_ERROR;

	for( auto ota_config : (*json_ota_config)["Configurations"].as<JsonArray>() ) {

		if ( ota_config.containsKey( "Board" ))
			board.assign( ota_config["Board"].as<const char *>() );

		if ( ota_config.containsKey( "Device" ))
			device.assign( ota_config["Device"].as<const char *>() );

		if ( ota_config.containsKey( "Config" ))
			config.assign( ota_config["Config"].as<const char *>() );

		if ( ota_config.containsKey( "Version" ))
			version.assign( ota_config["Version"].as<const char *>() );

		if (( !board.size() || ( board == board_name )) &&
			( !device.size() || ( device == device_name )) &&
			( !config.size() || ( config == config_name ))) {

					if ( !version.size() || ( version > current_version ))
						return ( action == DONT_DO_UPDATE ? UPDATE_AVAILABLE : do_ota_update( ota_config["URL"], action ));
			found_profile = true;
		}
	}
	return found_profile ? NO_UPDATE_AVAILABLE : NO_UPDATE_PROFILE_FOUND;
}

int AWSOTA::do_ota_update( const char *url, ActionType action )
{
	HTTPClient	http;
	int			http_status;
	
	if ( !http.begin( url ))
		return HTTP_FAILED;

	http_status = http.GET();

	if ( http_status != 200 ) {

		http.end();
		return http_status;
	}

	int total_length = http.getSize();

	if ( !Update.begin( UPDATE_SIZE_UNKNOWN )) {

		http.end();
		return OTA_UPDATE_FAIL;
	}

	std::array<uint8_t,1280>	buffer;

	WiFiClient *stream = http.getStreamPtr();

	int offset = 0;
	while ( http.connected() && offset < total_length ) {

		size_t	bytes_available = stream->available();
		if ( bytes_available > 0 ) {

			size_t bytes_to_read = min( bytes_available, buffer.max_size() );
			size_t bytes_read = stream->readBytes( buffer.data(), bytes_to_read );
			esp_task_wdt_reset();
			size_t bytes_written = Update.write( buffer.data(), bytes_read );
			if ( bytes_read != bytes_written )
				break;
			offset += bytes_written;
			if ( callback != nullptr )
				callback( offset, total_length );
		}
		esp_task_wdt_reset();
	}

	esp_task_wdt_reset();
	http.end();
	esp_task_wdt_reset();

	if ( offset == total_length ) {

		Update.end( true );
		esp_task_wdt_reset();
		delay( 1000 );
		if ( action == UPDATE_BUT_NO_BOOT )
			return UPDATE_OK;
		ESP.restart();
	}
	
	return WRITE_ERROR;
}

int AWSOTA::download_json( const char *url )
{
	HTTPClient	http;
	int			http_status;

	if ( !http.begin( url ))
		return HTTP_FAILED;

	if ( ( http_status = http.GET()) == 200 )
		deserialisation_status = deserializeJson( *json_ota_config, http.getString() );

	http.end();
	return http_status;
}

void AWSOTA::set_board_name( etl::string<24> &board )
{
	board_name = etl::string_view( board.data() );
}

void AWSOTA::set_callback( void (*_callback)(int, int) )
{
	callback = _callback;
}

void AWSOTA::set_config_name( etl::string<32> &config )
{
	config_name = etl::string_view( config.data() );
}

void AWSOTA::set_device_name( etl::string<18> &device )
{
	device_name = etl::string_view( device.data() );
}
