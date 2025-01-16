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

const std::array<std::string, 3>	Wind_vane::WIND_VANE_MODEL			= { "PR-3000-FXJT-N01 and compatible", "GD-FX-RS485", "VMS-3003-CFSFX-N01" };
const std::array<std::string, 3>	Wind_vane::WIND_VANE_DESCRIPTION	= { "Mechanical wind vane", "Mechanical wind vane", "Ultrasonic wind vane" };
const std::array<uint64_t,3>		Wind_vane::WIND_VANE_CMD			= { 0x020300000002c438, 0x020300000002c438, 0x010300000002c40b };
const std::array<uint16_t,3>		Wind_vane::WIND_VANE_SPEED			= { 4800, 9600, 4800 };

bool Wind_vane::initialise( SoftwareSerial *bus, byte model, bool _debug_mode )
{
	set_debug_mode( _debug_mode );
	set_name( WIND_VANE_MODEL[ model ].c_str() );
	set_description( WIND_VANE_DESCRIPTION[ model ].c_str() );
	set_driver_version( "1.0");

	return RS485Device::initialise( "WINDVANE  ", bus, GPIO_WIND_SENSOR_RX, GPIO_WIND_SENSOR_TX, GPIO_WIND_SENSOR_CTRL, WIND_VANE_CMD[ model ], WIND_VANE_SPEED[ model ] );
}

int16_t Wind_vane::get_wind_direction( bool verbose )
{
	if ( interrogate( verbose ) ) {

		wind_direction = ( get_answer()[5] << 8 ) + get_answer()[6];
		if ( get_debug_mode() && verbose )
			Serial.printf( "\n[WINDVANE  ] [DEBUG] Wind direction: %d°\n", wind_direction );

	} else

		wind_direction = -1;

	return wind_direction;
}
