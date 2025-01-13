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

const std::array<std::string, 3>	Anemometer::ANEMOMETER_MODEL		= { "PR-3000-FSJT-N01 and compatible", "GD-FS-RS485", "VMS-3003-CFSFX-N01" };
const std::array<std::string, 3>	Anemometer::ANEMOMETER_DESCRIPTION	= { "Mechanical anemometer", "Mechanical anemometer", "Ultrasonic anemometer" };
const std::array<uint64_t,3>		Anemometer::ANEMOMETER_CMD			= { 0x010300000001840a, 0x010300000001840a, 0x010300000002c40b };
const std::array<uint16_t,3>		Anemometer::ANEMOMETER_SPEED		= { 4800, 9600, 4800 };

bool Anemometer::initialise( SoftwareSerial *bus, uint32_t interval, byte _model, bool _debug_mode )
{
	model = _model;
	set_debug_mode( _debug_mode );

	wind_speeds_size = 2*60*1000 / interval;
	wind_speeds.resize( wind_speeds_size );

	set_name( ANEMOMETER_DESCRIPTION[ model ].c_str() );
	set_description( ANEMOMETER_DESCRIPTION[ model ].c_str() );
	set_driver_version( "1.0" );

	return RS485Device::initialise( "ANEMOMETER", bus, GPIO_WIND_SENSOR_RX, GPIO_WIND_SENSOR_TX, GPIO_WIND_SENSOR_CTRL, ANEMOMETER_CMD[ model ], ANEMOMETER_SPEED[ model ] );
}

float Anemometer::get_wind_speed( bool verbose )
{
	if ( interrogate( verbose ) ) {

		if ( model == 2 )
			wind_speed = static_cast<float>(get_answer()[4]) / 100.F;
		else
			wind_speed = static_cast<float>(get_answer()[4]) / 10.F;

		if ( get_debug_mode() && verbose )
			Serial.printf( "\n[ANEMOMETER] [DEBUG] Wind speed: %02.2f m/s\n", wind_speed );

		if ( wind_speed_index == wind_speeds_size )
			wind_speed_index = 0;
		wind_speeds[ wind_speed_index++ ] = wind_speed;

		return wind_speed;

	}

	if ( wind_speed_index == wind_speeds_size )
		wind_speed_index = 0;
	wind_speeds[ wind_speed_index++ ] = 0;

	return ( wind_speed = -1.F );
}

float Anemometer::get_wind_gust( void )
{
	if ( !wind_speeds_size )
		return wind_speed;
	return ( wind_gust = *std::max_element( wind_speeds.begin(), wind_speeds.end() ));
}
