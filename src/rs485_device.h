/*
  	rs485_device.h

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

#pragma once
#ifndef _rs485_device_H
#define	_rs485_device_H

#include <SoftwareSerial.h>
#include "device.h"

#define SEND    HIGH
#define RECV    LOW

class RS485Device : public Device {

	public:

								RS485Device( void ) = default;
		std::array<uint8_t,7> 	get_answer( void );
		bool					initialise( etl::string_view, SoftwareSerial *, uint8_t, uint8_t, uint8_t, uint64_t, uint16_t );
		bool					interrogate( bool );

	private:

		std::array<uint8_t,7>	answer;
   		uint16_t				bps					= 0;
		std::array<uint8_t,8>	cmd;
		uint8_t					ctrl_pin;
		etl::string_view		device_type;
		uint8_t					rx_pin;
		SoftwareSerial			*sensor_bus			= nullptr;
		uint8_t					tx_pin;

};

#endif
