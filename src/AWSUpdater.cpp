/*
  	AWSUpdater.cpp

	(c) 2024 F.Lesage

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
#include <ArduinoJson.h>
#include <Ethernet.h>
#include <SSLClient.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include "Embedded_Template_Library.h"
#include "etl/string.h"

#include "AWSUpdater.h"

int AWSUpdater::download_file( const char *root_ca, const char *server, const char *path, const char *package, const char *filename, const char *hash )
{
	etl::string<128>	end_point;
	File				file;
	HTTPClient 			http;
	uint8_t				http_code;
	etl::string<64>		local_filename;
	
	end_point = "https://";
	end_point += server;
	end_point += "/";
	end_point += path;
	end_point += "/";
	end_point += package;
	end_point += filename;

	local_filename = filename;
	local_filename += ".tmp";

	if ( LittleFS.exists( local_filename.data() ))
		LittleFS.remove( local_filename.data() );

	Serial.printf( "[UPDATER   ] [INFO ] Downloading file [%s] from update package [%s].\n", filename, package );

	if ( !http.begin( end_point.data(), root_ca )) {
	
       	Serial.printf( "[UPDATER   ] [ERROR] Cannot connect to file packager server, aborting.\n" );
		return false;
		
	}

	http.setFollowRedirects( HTTPC_FORCE_FOLLOW_REDIRECTS );
	esp_task_wdt_reset();
	http_code = http.GET();
	esp_task_wdt_reset();
	if ( http_code != 200 ) {
		
       	Serial.printf( "[UPDATER   ] [ERROR] Cannot download file from server (error code: %d), aborting.\n", http_code );
		http.end();
		return false;
	}

	if ( http.getSize() >= fs_free_space() ) {

		Serial.printf( "[UPDATER   ] [ERROR] No space left to save file, aborting.\n" );
		http.end();
		return false;
	}
	
	esp_task_wdt_reset();
	file = LittleFS.open( local_filename.data(), FILE_WRITE );
	if ( file ) {

		int sz = http.writeToStream( &file );
		http.end();
		file.close();
		file = LittleFS.open( local_filename.data(), FILE_READ );
		int sz2 = file.size();
		file.close();
		if ( sz == sz2 ) {

			LittleFS.remove( filename );
			LittleFS.rename( local_filename.data(), filename );
			Serial.printf( "[UPDATER   ] [INFO ] New version of file [%s] has been installed.\n", filename );
			return true;

		}
		Serial.printf( "[UPDATER   ] [ERROR] New version of file [%s] could not be installed.\n", filename );
		LittleFS.remove( local_filename.data() );	
		return false;

	}

	esp_task_wdt_reset();
	Serial.printf( "[UPDATER   ] [ERROR] Cannot create new version of file [%s].\n", filename );
	esp_task_wdt_reset();
	http.end();
	return false;
}

bool AWSUpdater::check_for_new_files( const char *current_version, const char *root_ca, const char *server, const char *path )
{
	uint8_t								http_code;
	HTTPClient 							http;
	ArduinoJson::DeserializationError 	ret;
	DynamicJsonDocument					json( 1000 );
	etl::string<128>					end_point;
	
	end_point = "https://";
	end_point += server;
	end_point += "/";
	end_point += path;
	end_point += "/";
	end_point += "ota_file_packages.json";

	Serial.printf( "[UPDATER   ] [INFO ] Checking availability of file package update.\n" );

	if ( !http.begin( end_point.data(), root_ca )) {
	
       	Serial.printf( "[UPDATER   ] [ERROR] Cannot connect to file package server, aborting.\n" );
		return false;
		
	}

	http.setFollowRedirects( HTTPC_FORCE_FOLLOW_REDIRECTS );
	http_code = http.GET();
	if ( http_code != 200 ) {
		
       	Serial.printf( "[UPDATER   ] [ERROR] Could not retrive file package manifest (error code: %d), aborting.\n", http_code );
		http.end();
		return false;

	}

	if ( ( ret = deserializeJson( json, http.getString() )) != DeserializationError::Ok ) {

		Serial.printf( "[UPDATER   ] [ERROR] Problem with manifest file (error: %d), aborting!\n", ret );
		http.end();
		return false;

	}

	http.end();

	if (!LittleFS.begin()) {

    	Serial.printf( "[UPDATER   ] [ERROR] Cannot access flash storage, aborting!\n" );
		return false;
	}

	JsonArray packages = json["packages"];
	for( JsonObject package : packages ) {
		
		const char *mv = package["min_version"];
		if ( strcmp( current_version, mv ) < 0 ) {

			const char *id = package["id"];
			Serial.printf( "[UPDATER   ] [INFO ] Found applicable file package [%s]\n", id );
			JsonArray files = package["files"];
			for( JsonObject file_item : files )
				for( JsonPair file : file_item )
					download_file( root_ca, server, path, id, file.key().c_str(), file.value().as<const char *>() );
			return true;

		}
	}

	Serial.printf( "[UPDATER   ] [INFO ] No applicable file package found.\n" );
	return true;
}

uint32_t AWSUpdater::fs_free_space( void )
{
	return LittleFS.totalBytes() - LittleFS.usedBytes();
}
