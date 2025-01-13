/*
  	wind_vane.h

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
#ifndef _wind_vane_H
#define	_wind_vane_H

#include <SoftwareSerial.h>
#include "rs485_device.h"

#define SEND	HIGH
#define RECV	LOW

class Wind_vane : public RS485Device {

	public:

		static const std::array<std::string, 3> WIND_VANE_MODEL;
		static const std::array<std::string, 3> WIND_VANE_DESCRIPTION;
		static const std::array<uint64_t,3>		WIND_VANE_CMD;
		static const std::array<uint16_t,3>		WIND_VANE_SPEED;

						Wind_vane( void ) = default;
		bool			initialise( SoftwareSerial *, byte, bool );
		int16_t			get_wind_direction( bool );

	private:

   		int16_t					wind_direction	= 0;
};

#endif
