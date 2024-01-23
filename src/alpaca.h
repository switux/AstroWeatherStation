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

#include <AsyncUDP_ESP32_W5500.hpp>
#include <SiderealPlanets.h>

#define	ALPACA_SERVER_PORT	8080

#define	CONFIGURED_DEVICES	4
#define	AWS_UUID			"ed0f1194-7cff-11ee-adf4-43fdf91ce20d"
#define	SAFETYMONITOR_UUID	"be358e98-7cff-11ee-8804-93595b17357e"
#define	DOME_UUID			"e3a90ace-7cff-11ee-b0f6-73db65e0b158"
#define	TELESCOPE_UUID		"17e70650-8733-11ee-aa35-8f2796da5b48"

#include "alpaca_dome.h"
#include "alpaca_safetymonitor.h"
#include "alpaca_observingconditions.h"

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

	// flawfinder: ignore
	char	DeviceName[32];
	// flawfinder: ignore
	char	DeviceType[20];
	uint32_t DeviceNumber;
	// flawfinder: ignore
	char	UniqueID[37];

} configured_device_t;

typedef enum
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

} ascom_error_t;

typedef enum
{
	NotAvailable,
	DeviceTimeout

} ascom_driver_error_t;

class ascom_device {

	protected:

		// flawfinder: ignore
		char			message_str[ 256 ];
		bool			debug_mode,
						is_connected = false;
		const char		*_description,
						*devicetype,
						*_driverinfo,
						*_driverversion,
						*_name,
						*_supportedactions;
		short			_interfaceversion;

	public:

			ascom_device( void );
		void default_bool( AsyncWebServerRequest *, const char *, bool );
		void description( AsyncWebServerRequest *, const char * );
		void device_error( AsyncWebServerRequest *, const char *, ascom_driver_error_t , char * );
		void driverinfo( AsyncWebServerRequest *, const char * );
		void driverversion( AsyncWebServerRequest *, const char * );
		void get_connected( AsyncWebServerRequest *, const char * );
		void interfaceversion( AsyncWebServerRequest *, const char * );
		void name( AsyncWebServerRequest *, const char * );
		void not_implemented( AsyncWebServerRequest *, const char *, const char * );
		void return_value( AsyncWebServerRequest *, const char *, byte  );
		void return_value( AsyncWebServerRequest *, const char *, double  );
		void supportedactions( AsyncWebServerRequest *, const char * );
};

class alpaca_safetymonitor : public ascom_device
{
	public:

		explicit alpaca_safetymonitor( bool );

		void issafe( AsyncWebServerRequest *, const char * );
		void set_connected( AsyncWebServerRequest *, const char * );
};

class alpaca_observingconditions : public ascom_device
{
	public:

			explicit alpaca_observingconditions( bool );

		void set_connected( AsyncWebServerRequest *request, const char * );
		void get_averageperiod( AsyncWebServerRequest *request, const char * );
		void set_averageperiod( AsyncWebServerRequest *request, const char * );
		void cloudcover( AsyncWebServerRequest *request, const char * );
		void dewpoint( AsyncWebServerRequest *request, const char * );
		void humidity( AsyncWebServerRequest *request, const char * );
		void pressure( AsyncWebServerRequest *request, const char * );
		void rainrate( AsyncWebServerRequest *request, const char * );
		void skybrightness( AsyncWebServerRequest *request, const char * );
		void skyquality( AsyncWebServerRequest *request, const char * );
		void skytemperature( AsyncWebServerRequest *request, const char * );
		void temperature( AsyncWebServerRequest *request, const char * );
		void winddirection( AsyncWebServerRequest *request, const char * );
		void windgust( AsyncWebServerRequest *request, const char * );
		void windspeed( AsyncWebServerRequest *request, const char * );
		void refresh( AsyncWebServerRequest *request, const char * );
		void sensordescription( AsyncWebServerRequest *request, const char * );
		void timesincelastupdate( AsyncWebServerRequest *request, const char * );

};

typedef enum
{
	Open,
	Closed,
	Opening,
	Closing,
	Error
} dome_shutter_status_t;

typedef struct alpaca_dome_t {

	dome_shutter_status_t	shutterstatus;

} alpaca_dome_t;

class alpaca_dome : public ascom_device
{
	private:

		dome_shutter_status_t	dome_shutter_status;

	public:

		explicit	alpaca_dome( bool );
		void		abortslew( AsyncWebServerRequest *, const char * );
		void		cansetshutter( AsyncWebServerRequest *, const char * );
		void		closeshutter( AsyncWebServerRequest *, const char * );
		void		openshutter( AsyncWebServerRequest *, const char * );
		void		set_connected( AsyncWebServerRequest *, const char * );
		void		slaved( AsyncWebServerRequest *, const char * );
		void		shutterstatus( AsyncWebServerRequest *, const char * );
};

class alpaca_telescope : public ascom_device
{
	private:
			SiderealPlanets	astro_lib;

			double	forced_latitude = -1,
					forced_longitude = -1,
					forced_altitude = -1;
	public:

		explicit alpaca_telescope( bool );
		void set_connected( AsyncWebServerRequest *, const char * );
		void siderealtime( AsyncWebServerRequest *, const char * );
		void siteelevation( AsyncWebServerRequest *, const char * );
		void set_siteelevation( AsyncWebServerRequest *, const char * );
		void sitelatitude( AsyncWebServerRequest *, const char * );
		void set_sitelatitude( AsyncWebServerRequest *, const char * );
		void sitelongitude( AsyncWebServerRequest *, const char * );
		void set_sitelongitude( AsyncWebServerRequest *, const char * );
		void trackingrates( AsyncWebServerRequest *, const char * );
		void utcdate( AsyncWebServerRequest *, const char * );
		void set_utcdate( AsyncWebServerRequest *, const char * );
		void axisrates( AsyncWebServerRequest *, const char * );
};

class alpaca_server {

	public:

		explicit		alpaca_server( bool );
		alpaca_server	*get_alpaca( void );
		bool			get_debug_mode( void );
		AsyncUDP		*get_discovery( void );
		void			loop( void );
		bool			start( IPAddress );

	private:

		bool			bad_request,
						debug_mode,
						server_up = false;

		AsyncWebServer 	*server;
		AsyncUDP		ascom_discovery;

		alpaca_dome					*dome;
		alpaca_observingconditions	*observing_conditions;
		alpaca_safetymonitor		*safety_monitor;
		alpaca_telescope			*telescope;

		ascom_error_t	transaction_status;

		// flawfinder: ignore
		char			buf[ 255 ] = {0},
						transaction_details[ 128 ];

		int				client_id = 0,
						client_transaction_id = 0,
						server_transaction_id = 0;

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
