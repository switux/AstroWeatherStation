/*
  	device.cpp

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

#include "device.h"

bool Device::get_debug_mode( void )
{
	return debug_mode;	
}

etl::string_view Device::get_description( void )
{
	return etl::string_view( description );
}

etl::string_view Device::get_driver_info( void )
{
	return etl::string_view( driver_info );
}

etl::string_view Device::get_driver_version( void )
{
	return etl::string_view( driver_version );
}

bool Device::get_initialised( void )
{
	return initialised;
}

etl::string_view Device::get_name( void )
{
	return etl::string_view( name );
}

void Device::set_debug_mode( bool b )
{
	debug_mode = b;
}

void Device::set_description( const char *str )
{
	description = etl::string<128>( str );
}

void Device::set_driver_version( const char *str )
{
	driver_version = etl::string<3>( str );
}

bool Device::set_initialised( bool b )
{
	return ( initialised = b );
}

void Device::set_name( const char *str )
{
	name = etl::string<128>( str );
}

void Device::uint64_t_to_uint8_t_array( uint64_t cmd, std::array<uint8_t,8> &cmd_array )
{
	uint8_t i = 0;
    for ( i = 0; i < 8; i++ )
		cmd_array[ i ] = (uint8_t)( ( cmd >> (56-(8*i))) & 0xff );
}
