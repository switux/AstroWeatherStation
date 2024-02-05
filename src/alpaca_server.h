/*
	alpaca_server.h

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
#ifndef _ALPACA_SERVER_H
#define _ALPACA_SERVER_H

#include <AsyncUDP_ESP32_W5500.hpp>

#define	ALPACA_SERVER_PORT	8080

#define	CONFIGURED_DEVICES	4
#define	AWS_UUID			"ed0f1194-7cff-11ee-adf4-43fdf91ce20d"
#define	SAFETYMONITOR_UUID	"be358e98-7cff-11ee-8804-93595b17357e"
#define	DOME_UUID			"e3a90ace-7cff-11ee-b0f6-73db65e0b158"
#define	TELESCOPE_UUID		"17e70650-8733-11ee-aa35-8f2796da5b48"

#include "alpaca_dome.h"
#include "alpaca_safetymonitor.h"
#include "alpaca_observingconditions.h"
#include "alpaca_telescope.h"

typedef struct configured_device_t {

	// flawfinder: ignore
	char	DeviceName[32];
	// flawfinder: ignore
	char	DeviceType[20];
	uint32_t DeviceNumber;
	// flawfinder: ignore
	char	UniqueID[37];

} configured_device_t;

enum struct ascom_error_t : byte
{
	Ok,
	PropertyOrMethodNotImplemented,
	InvalidValue,
	ValueNotSet,
	NotConnected,
	InvalidWhileParked,
	InvalidWhileSlaved,
	InvalidOperation,
	ActionNotImplemented

};
using ascom_error = ascom_error_t;

class alpaca_server {

	public:

		explicit		alpaca_server( void );
		alpaca_server	*get_alpaca( void );
		bool			get_debug_mode( void );
		AsyncUDP		*get_discovery( void );
		void			loop( void );
		bool			start( IPAddress, bool );

	private:

		bool			bad_request		= false;
		bool			debug_mode		= false;
		bool			server_up		= false;

		AsyncWebServer 	*server			= nullptr;
		AsyncUDP		ascom_discovery;

		alpaca_dome					*dome;
		alpaca_observingconditions	*observing_conditions;
		alpaca_safetymonitor		*safety_monitor;
		alpaca_telescope			*telescope;

		ascom_error		transaction_status	= ascom_error_t::NotConnected;

		// flawfinder: ignore
		char			buf[ 255 ] = {0};
		char			transaction_details[ 128 ];

		int				client_id = 0;
		int				client_transaction_id = 0;
		int				server_transaction_id = 0;

		void 			alpaca_getapiversions( AsyncWebServerRequest * );
		void			alpaca_getdescription( AsyncWebServerRequest * );
		void			alpaca_getconfigureddevices( AsyncWebServerRequest * );
		void 			alpaca_getsetup( AsyncWebServerRequest * );
		void 			dispatch_dome_request( AsyncWebServerRequest * );
		void			dispatch_observingconditions_request( AsyncWebServerRequest * );
		void 			dispatch_request( AsyncWebServerRequest * );
		void			dispatch_safetymonitor_request( AsyncWebServerRequest * );
		void 			dispatch_telescope_request( AsyncWebServerRequest * );
		bool		 	extract_transaction_details( AsyncWebServerRequest *, bool );
		void			does_not_exist( AsyncWebServerRequest * );
		void			get_config( AsyncWebServerRequest * );
		bool			get_configured_devices( char *, size_t );
		void			not_implemented( AsyncWebServerRequest * );
		void			not_implemented( AsyncWebServerRequest *, const char * );
		void			on_packet( AsyncUDPPacket  );
		void			send_file( AsyncWebServerRequest * );
};

#endif
