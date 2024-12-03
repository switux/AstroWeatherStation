/*
  	config_server.h

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

#pragma once
#ifndef _config_server_H
#define _config_server_H

#define _ASYNC_WEBSERVER_LOGLEVEL_		0	// NOSONAR
#define _ETHERNET_WEBSERVER_LOGLEVEL_	0	// NOSONAR

#include <AsyncTCP.h>
#include <Ethernet.h>
#include <SSLClient.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

class AWSWebServer {

	public:

				AWSWebServer( void ) = default;
		void attempt_ota_update( AsyncWebServerRequest * );
		void get_configuration( AsyncWebServerRequest * );
		void get_hw_configuration( AsyncWebServerRequest * );
		void get_station_data( AsyncWebServerRequest * );
		void get_root_ca( AsyncWebServerRequest * );
		bool initialise( bool );
		void stop( void );
		void start( void );

	private:

		AsyncWebServer 	*server		= nullptr;
		bool			debug_mode	= false;
		bool			initialised	= false;
		
		void		close_dome_shutter( AsyncWebServerRequest * );
		void 		get_lookout_rules_state( AsyncWebServerRequest * );
		void		get_uptime( AsyncWebServerRequest * );
		void		handle404( AsyncWebServerRequest * );
		void		index( AsyncWebServerRequest * );
		void		open_dome_shutter( AsyncWebServerRequest * );
		void		reboot( AsyncWebServerRequest * );
		void		resume_lookout( AsyncWebServerRequest * );
		void		rm_file( AsyncWebServerRequest * );
		const char 	*save_configuration( const char *json_string );
		void 		set_configuration( AsyncWebServerRequest *, JsonVariant & );
		void		send_file( AsyncWebServerRequest * );
		void		suspend_lookout( AsyncWebServerRequest * );

};

#endif
