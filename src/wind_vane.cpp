/*
  	wind_vane.cpp

	(c) 2023-2024 F.Lesage

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

const std::array<std::string, 3>	Wind_vane::WIND_VANE_MODEL			= { "PR-3000-FXJT-N01", "GD-FX-RS485", "VMS-3003-CFSFX-N01" };
const std::array<std::string, 3>	Wind_vane::WIND_VANE_DESCRIPTION	= { "Mechanical wind vane", "Mechanical wind vane", "Ultrasonic wind vane" };
const std::array<uint64_t,3>		Wind_vane::WIND_VANE_CMD			= { 0x010300000002c40b, 0x020300000002c438, 0x010300000002c40b };
const std::array<uint16_t,3>		Wind_vane::WIND_VANE_SPEED			= { 4800, 9600, 4800 };

Wind_vane::Wind_vane()
{
	pinMode( GPIO_WIND_SENSOR_CTRL, OUTPUT );
}

bool Wind_vane::initialise( SoftwareSerial *bus, byte model, bool _debug_mode )
{
	sensor_bus = bus;
	set_debug_mode( _debug_mode );
	set_name( WIND_VANE_MODEL[ model ].c_str() );
	set_description( WIND_VANE_DESCRIPTION[ model ].c_str() );
	set_driver_version( "1.0");

	uint64_t_to_uint8_t_array( WIND_VANE_CMD[ model ], cmd );

	if ( !bps )

		bps = WIND_VANE_SPEED[ model ];

	else {

		if ( bps != WIND_VANE_SPEED[ model ] ) {

			bps = WIND_VANE_SPEED[ model ];
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

	answer.fill( 0 );

	if ( get_debug_mode() ) {

		if ( verbose ) {

			etl::string<8> str( cmd.begin(), cmd.end() );
			Serial.printf( "[DEBUG] Sending command to the wind vane: %s\n", str.data() );

		} else

			Serial.printf( "[DEBUG] Probing wind vane.\n" );
	}

	while ( answer[1] != 0x03 ) {

		digitalWrite( GPIO_WIND_SENSOR_CTRL, SEND );

		sensor_bus->write( cmd.data(), cmd.max_size() );
		sensor_bus->flush();

		digitalWrite( GPIO_WIND_SENSOR_CTRL, RECV );
		sensor_bus->readBytes( answer.data(), answer.max_size() );

		if ( get_debug_mode() && verbose ) {

			etl::string<7> str( answer.begin(), answer.end() );
			Serial.printf( "[DEBUG] Anemometer answer: %s", str.data() );
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
