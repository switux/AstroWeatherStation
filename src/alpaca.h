/*
	alpaca.h

	ASCOM ALPACA Server for the AstroWeatherStation (c) 2023 F.Lesage

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
#ifndef _ALPACA_H
#define _ALPACA_H

#undef CONFIG_DISABLE_HAL_LOCKS
#define _ASYNC_WEBSERVER_LOGLEVEL_       0
#define _ETHERNET_WEBSERVER_LOGLEVEL_      0

#define	ALPACA_SERVER_PORT	8080		// The actual stuff

#define	AWS_UUID			"ed0f1194-7cff-11ee-adf4-43fdf91ce20d"
#define	SAFETYMONITOR_UUID	"be358e98-7cff-11ee-8804-93595b17357e"
#define	DOME_UUID			"e3a90ace-7cff-11ee-b0f6-73db65e0b158"

typedef enum 
{
	Camera,
	CoverCalibrator,
	Dome,
	FilterWheel,
	Focuser,
	ObservingConditions,
	Rotator,
	SafetyMonitor,
	Switch,
	Telescope,
	Video
} ascom_device_t;

typedef struct configured_device_t {
	
	char	DeviceName[32];
	char	DeviceType[20];
	uint32_t DeviceNumber;
	char	UniqueID[37];
	
} configured_device_t;

class alpaca_server {

	public:
		alpaca_server( void );
		bool start( IPAddress, bool );
		void loop( void );
		void alpaca_getapiversions( AsyncWebServerRequest * );
		void alpaca_getdescription( AsyncWebServerRequest * );
		void alpaca_getconfigureddevices( AsyncWebServerRequest * );
		void safetymonitor_issafe( AsyncWebServerRequest * );
		void handle404( AsyncWebServerRequest * );
		void alpaca_getsetup( AsyncWebServerRequest * );
		alpaca_server *get_alpaca( void );
		void get_config( AsyncWebServerRequest * );
		void on_packet( AsyncUDPPacket  );

		void send_file( AsyncWebServerRequest * );
		void stop( void );
		void stop_discovery( void );

		AsyncUDP *get_discovery( void );
		bool	get_debug_mode( void );
		
	private:

		bool 				server_up = false;
		bool				debug_mode;
		AsyncWebServer 		*server;
		AsyncUDP			ascom_discovery;
		
		char				buf[ 255 ] = {0};
		unsigned int		client_id = 0;
		unsigned int		client_transaction_id = 0;
		unsigned int		server_transaction_id = 0;
		const char 			*get_transaction_details( AsyncWebServerRequest * );

};

#endif
