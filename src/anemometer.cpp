/*
  	anemometer.cpp

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
#include <vector>

#include "defaults.h"
#include "gpio_config.h"
#include "device.h"
#include "anemometer.h"

const std::array<std::string, 3>	Anemometer::ANEMOMETER_MODEL		= { "PR-3000-FSJT-N01", "GD-FS-RS485", "VMS-3003-CFSFX-N01" };
const std::array<std::string, 3>	Anemometer::ANEMOMETER_DESCRIPTION	= { "Mechanical anemometer", "Mechanical anemometer", "Ultrasonic anemometer" };
const std::array<uint64_t,3>		Anemometer::ANEMOMETER_CMD			= { 0x010300000001840a, 0x010300000001840a, 0x010300000002c40b };
const std::array<uint16_t,3>		Anemometer::ANEMOMETER_SPEED		= { 4800, 9600, 4800 };

Anemometer::Anemometer( void )
{
	pinMode( GPIO_WIND_SENSOR_CTRL, OUTPUT );
	set_driver_version( "1.0" );
}

bool Anemometer::initialise( SoftwareSerial *bus, uint32_t interval, byte _model, bool _debug_mode )
{
	model = _model;
	sensor_bus = bus;
	set_debug_mode( _debug_mode );

	wind_speeds_size = 2*60*1000 / interval;
	wind_speeds.resize( wind_speeds_size );

	set_name( ANEMOMETER_DESCRIPTION[ model ].c_str() );
	set_description( ANEMOMETER_DESCRIPTION[ model ].c_str() );
	
	uint64_t_to_uint8_t_array( ANEMOMETER_CMD[ model ], cmd );

	if ( !bps )

		bps = ANEMOMETER_SPEED[ model ];

	else {

		if ( bps != ANEMOMETER_SPEED[ model ] ) {

			bps = ANEMOMETER_SPEED[ model ];
			sensor_bus->end();

		} else

			return set_initialised ( get_wind_speed( false ) >= 0 );
	}

	sensor_bus->begin( bps, EspSoftwareSerial::SWSERIAL_8N1, GPIO_WIND_SENSOR_RX, GPIO_WIND_SENSOR_TX );
	return set_initialised ( get_wind_speed( false ) >= 0 );
}

float Anemometer::get_wind_speed( bool verbose )
{
	byte	i = 0;
	byte	j;

	answer.fill( 0 );

	if ( get_debug_mode() ) {

		if ( verbose ) {

			Serial.printf( "[DEBUG] Sending command to the anemometer:" );
			for ( j = 0; j < 8; Serial.printf( " %02x", cmd[ j++ ] ));
			Serial.printf( "\n" );

		} else

			Serial.printf( "[DEBUG] Probing anemometer.\n" );
	}

	while ( answer[1] != 0x03 ) {

		digitalWrite( GPIO_WIND_SENSOR_CTRL, SEND );
		sensor_bus->write( cmd.data(), cmd.max_size() );
		sensor_bus->flush();

		digitalWrite( GPIO_WIND_SENSOR_CTRL, RECV );
		sensor_bus->readBytes( answer.data(), answer.max_size() );

		if ( get_debug_mode() && verbose ) {

			Serial.print( "[DEBUG] Anemometer answer: " );
			for ( j = 0; j < 7; j++ )
				Serial.printf( "%02x ", answer[j] );

		}

		if ( answer[1] == 0x03 ) {

			wind_speed = static_cast<float>(answer[4]) / 10.F;

			if ( get_debug_mode() && verbose )
				Serial.printf( "\n[DEBUG] Wind speed: %02.2f m/s\n", wind_speed );

		} else {

			if ( get_debug_mode() && verbose )
				Serial.printf( "(Error).\n" );
			delay( 500 );
		}

		if ( ++i == 3 ) {

			if ( wind_speed_index == wind_speeds_size )
				wind_speed_index = 0;
			wind_speeds[ wind_speed_index++ ] = 0;
			return ( wind_speed = -1.F );
		}
	}

	if ( wind_speed_index == wind_speeds_size )
		wind_speed_index = 0;
	wind_speeds[ wind_speed_index++ ] = wind_speed;

	return wind_speed;
}

float Anemometer::get_wind_gust( void )
{
	return ( wind_gust = *std::max_element( wind_speeds.begin(), wind_speeds.end() ));
}
