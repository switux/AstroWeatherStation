/*
  	AWSOTA.h

	(c) 2024 F.Lesage and inspired from ESP32-OTA-Pull by M.Hart

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

enum struct ota_action_t: int {

	CHECK_ONLY,
	UPDATE_ONLY,
	UPDATE_AND_BOOT
};

enum struct ota_status_t : int {

	UPDATE_AVAILABLE = -3,
	NO_UPDATE_PROFILE_FOUND = -2,
	NO_UPDATE_AVAILABLE = -1,
	UPDATE_OK = 0,
	HTTP_FAILED = 1,
	WRITE_ERROR = 2,
	JSON_PROBLEM = 3,
	OTA_UPDATE_FAIL = 4,
	CONFIG_ERROR = 5,
	UNKNOWN = 6
};

class AWSOTA {

	public:

			AWSOTA( void );
		ota_status_t	check_for_update( const char *, const char *root_ca, etl::string<26> &, ota_action_t );
		void			set_aws_board_id( etl::string<24> & );
		void			set_aws_config( etl::string<32> & );
		void			set_aws_device_id( etl::string<18> & );
		void			set_progress_callback( std::function<void(int, int)> );

	private:

		etl::string_view				aws_board_id;
		etl::string_view				aws_config;
		etl::string_view				aws_device_id;
		DeserializationError			deserialisation_status;
		int								http_status;
		DynamicJsonDocument				*json_ota_config		= nullptr;
	    std::function<void (int, int)>	progress_callback		= nullptr;
		ota_status_t					status_code				= ota_status_t::UNKNOWN;

		bool	do_ota_update( const char *, const char *, ota_action_t );
		bool	download_json( const char *, const char * );
		void	save_firmware_sha256( const char * );

};

#endif
