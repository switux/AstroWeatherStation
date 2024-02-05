/*
	alpaca_device.h

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
#ifndef _ALPACA_DEVICE_H
#define _ALPACA_DEVICE_H

enum struct ascom_device_type : byte
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
};
using ascom_device_t = ascom_device_type;

enum struct ascom_driver_error_t : byte
{
	NotAvailable,
	DeviceTimeout

};
using ascom_driver_error = ascom_driver_error_t;

class alpaca_device {

	protected:

		// flawfinder: ignore
		char			message_str[ 256 ];
		bool			debug_mode			= false;
		bool			is_connected		= false;
		const char		*_description		= nullptr;
		const char		*devicetype			= nullptr;
		const char		*_driverinfo		= nullptr;
		const char		*_driverversion		= nullptr;
		const char		*_name				= nullptr;
		const char		*_supportedactions	= nullptr;
		short			_interfaceversion	= 0;

	public:

			alpaca_device( void );
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

#endif
