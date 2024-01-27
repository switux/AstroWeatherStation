/*
  	AWSWind.cpp

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
#include "AWSWind.h"

const std::array<std::string, 3> AWSWindSensor::_anemometer_model = { "PR-3000-FSJT-N01", "GD-FS-RS485", "VMS-3003-CFSFX-N01" };
const uint64_t _anemometer_cmd[3] = { 0x010300000001840a, 0x010300000001840a, 0x010300000002c40b };
const uint16_t _anemometer_speed[3] = { 4800, 9600, 4800 };

const std::array<std::string, 3> AWSWindSensor::_windvane_model = { "PR-3000-FXJT-N01", "GD-FX-RS485", "VMS-3003-CFSFX-N01" };
const uint64_t _windvane_cmd[3] = { 0x010300000002c40b, 0x020300000002c438, 0x010300000002c40b };
const uint16_t _windvane_speed[3] = { 4800, 9600, 4800 };

AWSWindSensor::AWSWindSensor( uint32_t _polling_ms_interval, bool _debug_mode ) :
	_anemometer_initialised( false ),
	debug_mode( _debug_mode ),
	_wind_vane_initialised( false ),
	wind_speed_index( 0 ),
	wind_speeds_size( 2*60*1000 / _polling_ms_interval ),
	wind_gust( 0 ),
	wind_speed( 0.F ),
	wind_speeds( static_cast<float *>( malloc( wind_speeds_size * sizeof( float )) ) ),
	wind_direction( 0 ),
	bps( 0 ),
	polling_ms_interval( _polling_ms_interval ),
	sensor_bus( new SoftwareSerial( GPIO_WIND_SENSOR_RX, GPIO_WIND_SENSOR_TX ) )
{
	memset( anemometer_cmd, 0, 8 );
	memset( wind_vane_cmd, 0, 8 );
	memset( answer, 0, 7 );
	pinMode( GPIO_WIND_SENSOR_CTRL, OUTPUT );
}

AWSWindSensor::~AWSWindSensor( void )
{
	free( wind_speeds );
}

bool AWSWindSensor::anemometer_initialised( void )
{
	return _anemometer_initialised;
}

bool AWSWindSensor::wind_vane_initialised( void )
{
	return _wind_vane_initialised;
}

bool AWSWindSensor::initialise_anemometer( byte model )
{
	anemometer_model = model;
	uint64_t_to_uint8_t_array( _anemometer_cmd[ model ], anemometer_cmd );

	if ( !bps )

		bps = _anemometer_speed[ model ];

	else {

		if ( bps != _anemometer_speed[ model ] ) {

			bps = _anemometer_speed[ model ];
			sensor_bus->end();
			_wind_vane_initialised = false;

		} else

			return ( _anemometer_initialised = ( read_anemometer( false ) >= 0 ));
	}

	sensor_bus->begin( bps );
	return ( _anemometer_initialised = ( read_anemometer( false ) >= 0 ));
}

bool AWSWindSensor::initialise_wind_vane( byte model )
{
	uint64_t_to_uint8_t_array( _windvane_cmd[ model ], wind_vane_cmd );

	if ( !bps )

		bps = _windvane_speed[ model ];

	else {

		if ( bps != _windvane_speed[ model ] ) {

			bps = _windvane_speed[ model ];
			_anemometer_initialised = false;
			sensor_bus->end();

		} else

			return ( _wind_vane_initialised = ( read_wind_vane( false ) >= 0 ));
	}

	sensor_bus->begin( bps );
	return ( _wind_vane_initialised = ( read_wind_vane( false ) >= 0 ));
}

float AWSWindSensor::read_anemometer( bool verbose )
{
	byte	i = 0;
	byte	j;

	memset( answer, 0, 7 );

	if ( debug_mode ) {

		if ( verbose ) {

			Serial.printf( "[DEBUG] Sending command to the anemometer:" );
			for ( j = 0; j < 8; Serial.printf( " %02x", anemometer_cmd[ j++ ] ));
			Serial.printf( "\n" );

		} else

			Serial.printf( "[DEBUG] Probing anemometer.\n" );
	}

	while ( answer[1] != 0x03 ) {

		digitalWrite( GPIO_WIND_SENSOR_CTRL, SEND );
		sensor_bus->write( anemometer_cmd, 8 );
		sensor_bus->flush();

		digitalWrite( GPIO_WIND_SENSOR_CTRL, RECV );
		sensor_bus->readBytes( answer, 7 );

		if ( debug_mode && verbose ) {

			Serial.print( "[DEBUG] Anemometer answer: " );
			for ( j = 0; j < 7; j++ )
				Serial.printf( "%02x ", answer[j] );

		}

		if ( answer[1] == 0x03 ) {

			if ( anemometer_model != 0x02 )

				wind_speed = static_cast<float>(answer[4]) / 10.F;

			else {

				wind_speed = static_cast<float>( (answer[3]<< 8) + answer[4] ) / 100.F;
				wind_direction = ( answer[5] << 8 ) + answer[6];

				if ( debug_mode && verbose )
					Serial.printf( "\n[DEBUG] Wind direction: %d°", wind_direction );
			}

			if ( debug_mode && verbose )
				Serial.printf( "\n[DEBUG] Wind speed: %02.2f m/s\n", wind_speed );

		} else {

			if ( debug_mode && verbose )
				Serial.printf( "(Error).\n" );
			delay( 500 );
		}

		if ( ++i == WIND_SENSOR_MAX_TRIES ) {

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

float AWSWindSensor::get_wind_gust( void )
{
	return ( wind_gust = *std::max_element( wind_speeds, wind_speeds + wind_speeds_size ));
}

int16_t AWSWindSensor::read_wind_vane( bool verbose  )
{
	byte	i = 0;
	byte	j;

	memset( answer, 0, 7 );

	if ( debug_mode ) {

		if ( verbose ) {

			Serial.printf( "[DEBUG] Sending command to the wind vane:" );
			for ( j = 0; j < 8; Serial.printf( " %02x", wind_vane_cmd[ j++ ] ));
			Serial.printf( "\n" );

		} else

			Serial.printf( "[DEBUG] Probing wind vane.\n" );
	}

	while ( answer[1] != 0x03 ) {

		digitalWrite( GPIO_WIND_SENSOR_CTRL, SEND );

		sensor_bus->write( wind_vane_cmd, 8 );
		sensor_bus->flush();

		digitalWrite( GPIO_WIND_SENSOR_CTRL, RECV );
		sensor_bus->readBytes( answer, 7 );

		if ( debug_mode && verbose ) {

			Serial.print( "[DEBUG] Wind vane answer : " );
			for ( j = 0; j < 7; j++ )
				Serial.printf( "%02x ", answer[j] );
		}

		if ( answer[1] == 0x03 ) {

			wind_direction = ( answer[5] << 8 ) + answer[6];

			if ( debug_mode && verbose )
				Serial.printf( "\n[DEBUG] Wind direction: %d°\n", wind_direction );

		} else {

			if ( debug_mode && verbose )
				Serial.printf( "(Error).\n" );
			delay( 500 );
		}

		if ( ++i == WIND_SENSOR_MAX_TRIES )
			return ( wind_direction = -1 );
	}
	return wind_direction;
}

void AWSWindSensor::uint64_t_to_uint8_t_array( uint64_t cmd, uint8_t *cmd_array )
{
	uint8_t i = 0;
    for ( i = 0; i < 8; i++ )
		cmd_array[ i ] = (uint8_t)( ( cmd >> (56-(8*i))) & 0xff );
}
