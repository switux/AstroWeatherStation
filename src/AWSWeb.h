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
		bool initialise( uint32_t );


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
