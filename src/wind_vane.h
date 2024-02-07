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

#define SEND	HIGH
#define RECV	LOW

class Wind_vane : public Device {

	public:

		static const std::array<std::string, 3> WIND_VANE_MODEL;
		static const std::array<std::string, 3> WIND_VANE_DESCRIPTION;
		
				Wind_vane( void );
		bool			initialise( SoftwareSerial *, byte, bool );
		int16_t			get_wind_direction( bool );

	private:

   		int16_t			wind_direction	= 0;
   		uint16_t		bps				= 0;
		uint8_t			answer[7];
		uint8_t			cmd[8];
		SoftwareSerial	*sensor_bus		= nullptr;

		void uint64_t_to_uint8_t_array( uint64_t cmd, uint8_t *cmd_array );
};

#endif
