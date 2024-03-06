/*
	alpaca_device.h

	ASCOM ALPACA Server for the AstroWeatherStation (c) 2023-2024 F.Lesage

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

#include "Embedded_Template_Library.h"
#include "etl/string.h"

enum struct ascom_driver_error_t : byte
{
	NotAvailable,
	DeviceTimeout

};
using ascom_driver_error = ascom_driver_error_t;

using alpaca_device_description_t	= etl::string<68>;
using alpaca_driver_info_t			= etl::string<128>;
using alpaca_driver_version_t		= etl::string<3>;
using alpaca_device_name_t			= etl::string<64>;
using alpaca_supported_actions_t	= etl::string<128>;
using alpaca_interface_version_t	= int32_t;

class alpaca_device {

	private:

		bool						debug_mode			= false;
		bool						is_connected		= false;
//		std::shared_ptr<Device>		device;
		alpaca_device_description_t	description;
		alpaca_driver_info_t		driverinfo;
		alpaca_driver_version_t		driverversion;
		alpaca_device_name_t		name;
		alpaca_supported_actions_t	supportedactions;
		alpaca_interface_version_t	interfaceversion	= 0;

	public:

		explicit 	alpaca_device( alpaca_interface_version_t );
//		void		attach_device( std::shared_ptr<Device> _device );
		void		device_error( AsyncWebServerRequest *, const char *, ascom_driver_error_t , char * );
		bool		get_debug_mode( void );
		bool		get_is_connected( void );
		void		not_implemented( AsyncWebServerRequest *, const char *, const char * );
		void		send_connected( AsyncWebServerRequest *, const char * );
		bool		send_description( AsyncWebServerRequest *, const char * );
		bool		send_driverinfo( AsyncWebServerRequest *, const char * );
		bool		send_driverversion( AsyncWebServerRequest *, const char * );
		bool		send_interfaceversion( AsyncWebServerRequest *, const char * );
		bool		send_name( AsyncWebServerRequest *, const char * );
		bool		send_supportedactions( AsyncWebServerRequest *, const char * );
		void		send_value( AsyncWebServerRequest *, const char *, bool );
		void		send_value( AsyncWebServerRequest *, const char *, byte );
		void		send_value( AsyncWebServerRequest *, const char *, float );
		void		set_debug_mode( bool );
		void		set_description( char * );
		void		set_device_name( char * );
		void		set_driver_info( char * );
		void		set_driver_version( char * );
		void		set_is_connected( bool );
		void		set_supported_actions( char * );

};

#endif
