/*	
  	AWSWeb.h
  	
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

#pragma once
#ifndef _AWSWebServer_H
#define _AWSWebServer_H


class AWSWebServer {

	public:

		AWSWebServer( void );
		~AWSWebServer( void );
		
		void get_configuration( AsyncWebServerRequest * );
		void get_data( AsyncWebServerRequest * );
		void get_root_ca( AsyncWebServerRequest * );
		bool initialise( bool );

	private:

		AsyncWebServer 	*server;
		bool			debug_mode;

		void		get_uptime( AsyncWebServerRequest * );
		void		handle404( AsyncWebServerRequest * );
		void		index( AsyncWebServerRequest * );
		void		reboot( AsyncWebServerRequest * );
		void		reset_config_parameter( AsyncWebServerRequest * );
		const char 	*save_configuration( const char *json_string );
		void 		set_configuration( AsyncWebServerRequest *, JsonVariant & );
		void		send_file( AsyncWebServerRequest * );



};

#endif
