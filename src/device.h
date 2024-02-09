/*
  	device.h

	(c) 2024 F.Lesage

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
#ifndef _device_h
#define	_device_h

#include "Embedded_Template_Library.h"
#include "etl/string.h"

class Device {

	public:
							Device( void ) = default;
		etl::string_view 	get_description( void );
		etl::string_view	get_driver_info( void );
		etl::string_view	get_driver_version( void );
		bool				get_initialised( void );
		etl::string_view	get_name( void );
		bool				get_debug_mode( void );
		void				set_debug_mode( bool );
		void				set_description( const char * );
		void				set_driver_version( const char * );
		bool				set_initialised( bool );
		void				set_name( const char * );
		void				uint64_t_to_uint8_t_array( uint64_t, std::array<uint8_t,8> & );

	private:

		bool				debug_mode		= false;
		etl::string<128>	description;
		etl::string<3>		driver_version;
		etl::string<128>	driver_info;
		bool				initialised		= false;
		etl::string<128>	name;
};

#endif
