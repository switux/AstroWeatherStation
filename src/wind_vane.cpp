/*
  	wind_vane.cpp

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

#include <SoftwareSerial.h>

#include "defaults.h"
#include "gpio_config.h"
#include "device.h"
#include "wind_vane.h"

const std::array<std::string, 3> Wind_vane::_windvane_model = { "PR-3000-FXJT-N01", "GD-FX-RS485", "VMS-3003-CFSFX-N01" };
const std::array<std::string, 3> Wind_vane::_windvane_description = { "Mechanical wind vane", "Mechanical wind vane", "Ultrasonic wind vane" };
const uint64_t _windvane_cmd[3] = { 0x010300000002c40b, 0x020300000002c438, 0x010300000002c40b };
const uint16_t _windvane_speed[3] = { 4800, 9600, 4800 };

Wind_vane::Wind_vane()
{
	memset( cmd, 0, 8 );
	memset( answer, 0, 7 );
	pinMode( GPIO_WIND_SENSOR_CTRL, OUTPUT );
}

bool Wind_vane::initialise( SoftwareSerial *bus, byte model, bool _debug_mode )
{
	sensor_bus = bus;
	set_debug_mode( _debug_mode );
	set_name( _windvane_model[ model ].c_str() );
	set_description( _windvane_description[ model ].c_str() );
	set_driver_version( "1.0");

	uint64_t_to_uint8_t_array( _windvane_cmd[ model ], cmd );

	if ( !bps )

		bps = _windvane_speed[ model ];

	else {

		if ( bps != _windvane_speed[ model ] ) {

			bps = _windvane_speed[ model ];
			sensor_bus->end();

		} else

			return set_initialised( get_wind_direction( false ) >= 0 );
	}

	sensor_bus->begin( bps, EspSoftwareSerial::SWSERIAL_8N1, GPIO_WIND_SENSOR_RX, GPIO_WIND_SENSOR_TX );
	return set_initialised( get_wind_direction( false ) >= 0 );
}

int16_t Wind_vane::get_wind_direction( bool verbose  )
{
	byte	i = 0;
	byte	j;

	memset( answer, 0, 7 );

	if ( get_debug_mode() ) {

		if ( verbose ) {

			Serial.printf( "[DEBUG] Sending command to the wind vane:" );
			for ( j = 0; j < 8; Serial.printf( " %02x", cmd[ j++ ] ));
			Serial.printf( "\n" );

		} else

			Serial.printf( "[DEBUG] Probing wind vane.\n" );
	}

	while ( answer[1] != 0x03 ) {

		digitalWrite( GPIO_WIND_SENSOR_CTRL, SEND );

		sensor_bus->write( cmd, 8 );
		sensor_bus->flush();

		digitalWrite( GPIO_WIND_SENSOR_CTRL, RECV );
		sensor_bus->readBytes( answer, 7 );

		if ( get_debug_mode() && verbose ) {

			Serial.print( "[DEBUG] Wind vane answer : " );
			for ( j = 0; j < 7; j++ )
				Serial.printf( "%02x ", answer[j] );
		}

		if ( answer[1] == 0x03 ) {

			wind_direction = ( answer[5] << 8 ) + answer[6];

			if ( get_debug_mode() && verbose )
				Serial.printf( "\n[DEBUG] Wind direction: %d°\n", wind_direction );

		} else {

			if ( get_debug_mode() && verbose )
				Serial.printf( "(Error).\n" );
			delay( 500 );
		}

		if ( ++i == 3 )
			return ( wind_direction = -1 );
	}
	return wind_direction;
}

void Wind_vane::uint64_t_to_uint8_t_array( uint64_t cmd, uint8_t *cmd_array )
{
	uint8_t i = 0;
    for ( i = 0; i < 8; i++ )
		cmd_array[ i ] = (uint8_t)( ( cmd >> (56-(8*i))) & 0xff );
}
