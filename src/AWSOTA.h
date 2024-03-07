/*
  	AWSOTA.h

	(c) 2024 F.Lesage and heavily inspired from ESP32-OTA-Pull by M.Hart

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

#pragma once
#ifndef _AWSOTA_h
#define _AWSOTA_h

#include <ArduinoJson.h>

class AWSOTA {

	public:

		enum ActionType {
			DONT_DO_UPDATE,
			UPDATE_BUT_NO_BOOT,
			UPDATE_AND_BOOT
		};

		enum ErrorCode {
			UPDATE_AVAILABLE = -3,
			NO_UPDATE_PROFILE_FOUND = -2,
			NO_UPDATE_AVAILABLE = -1,
			UPDATE_OK = 0,
			HTTP_FAILED = 1,
			WRITE_ERROR = 2,
			JSON_PROBLEM = 3,
			OTA_UPDATE_FAIL = 4,
			CONFIG_ERROR = 5
		};

			AWSOTA( void );
		int check_for_update( const char *, etl::string<26> &, ActionType );
		void set_board_name( etl::string<24> & );
		void set_config_name( etl::string<32> & );
		void set_device_name( etl::string<18> & );
		void set_callback( void (*)( int, int ) );

	private:

		etl::string_view		board_name;
	    void 					(*callback)( int, int )		= nullptr;
		etl::string_view		config_name;
		DeserializationError	deserialisation_status;
		etl::string_view		device_name;
		DynamicJsonDocument		*json_ota_config			= nullptr;

		int do_ota_update( const char *, ActionType );
		int download_json( const char * );

};

#endif
