/*
  	rs486_device.cpp

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


#include "rs485_device.h"

std::array<uint8_t,7> RS485Device::get_answer( void )
{
	return answer;
}

bool RS485Device::initialise( etl::string_view devtype, SoftwareSerial *bus, uint8_t rx, uint8_t tx, uint8_t ctrl, uint64_t command, uint16_t spd )
{
	sensor_bus = bus;
	device_type = devtype;
	rx_pin = rx;
	tx_pin = tx;
	ctrl_pin = ctrl;

	pinMode( ctrl, OUTPUT );

	uint64_t_to_uint8_t_array( command, cmd );

	if ( !bps )

		bps = spd;

	else {

		if ( bps != spd ) {

			bps = spd;
			sensor_bus->end();

		} else

			return set_initialised ( interrogate( true ) );
	}

	sensor_bus->begin( bps, EspSoftwareSerial::SWSERIAL_8N1, rx_pin, tx_pin );
	return set_initialised ( interrogate( true ) );
}

bool RS485Device::interrogate( bool verbose )
{
	byte	i = 0;
	byte	j;

	answer.fill( 0 );

	if ( get_debug_mode() ) {

		if ( verbose ) {

			etl::string<8> str( cmd.begin(), cmd.end() );
			Serial.printf( "[%s] [DEBUG] Sending command: %s\n", device_type.data(), str.data() );

		} else

			Serial.printf( "[%s] [DEBUG] Probing.\n", device_type.data() );
	}

	while ( answer[1] != 0x03 ) {

		digitalWrite( ctrl_pin, SEND );
		sensor_bus->write( cmd.data(), cmd.max_size() );
		sensor_bus->flush();

		digitalWrite( ctrl_pin, RECV );
		sensor_bus->readBytes( answer.data(), answer.max_size() );

		if ( get_debug_mode() && verbose ) {

			etl::string<7> str( answer.begin(), answer.end() );
			Serial.printf( "[%s] [DEBUG] Answer: %s", device_type.data(), str.data() );

		}

		if ( answer[1] == 0x03 ) {

			return true;

		} else {

			if ( get_debug_mode() && verbose )
				Serial.printf( "(Error).\n" );
			delay( 500 );
		}

		if ( ++i == 3 ) {

			return false;
		}
	}
}
