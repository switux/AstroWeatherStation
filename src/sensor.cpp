/*
  	sensor.cpp

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

#include "sensor.h"

Sensor::Sensor( void )
{
}

bool Sensor::get_debug_mode( void )
{
	return debug_mode;	
}

etl::string_view Sensor::get_description( void )
{
	return etl::string_view( description );
}

etl::string_view Sensor::get_driver_info( void )
{
	return etl::string_view( driver_info );
}

etl::string_view Sensor::get_driver_version( void )
{
	return etl::string_view( driver_version );
}

bool Sensor::get_initialised( void )
{
	return initialised;
}

etl::string_view Sensor::get_name( void )
{
	return etl::string_view( name );
}

void Sensor::set_debug_mode( bool b )
{
	debug_mode = b;
}

void Sensor::set_description( const char *str )
{
	description = etl::string<128>( str );
}

void Sensor::set_driver_version( const char *str )
{
	driver_version = etl::string<3>( str );
}

bool Sensor::set_initialised( bool b )
{
	return ( initialised = b );
}

void Sensor::set_name( const char *str )
{
	name = etl::string<128>( str );
}
