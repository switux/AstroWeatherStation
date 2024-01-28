/*
  	AWSWind.h

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

#pragma once
#ifndef _AWS_WIND_H
#define	_AWS_WIND_H

#include <SoftwareSerial.h>

#define SEND	HIGH
#define RECV	LOW

const	byte WIND_SENSOR_MAX_TRIES = 3;		// Tests have shown that if there is no valid answer after 3 attempts, the sensor is not available

class AWSWindSensor {

	public:

		static const std::array<std::string, 3> _anemometer_model;
		static const std::array<std::string, 3> _windvane_model;
		
				AWSWindSensor( uint32_t, bool );
				~AWSWindSensor( void );
		bool	anemometer_initialised( void );
		float	get_wind_gust( void );
		bool	initialise_anemometer( byte );
		bool	initialise_wind_vane( byte );
		float	read_anemometer( bool );
		int16_t	read_wind_vane( bool );
		bool	wind_vane_initialised( void );

	private:

   		bool			_anemometer_initialised;
		bool			debug_mode;
		bool			_wind_vane_initialised;
	   	byte			anemometer_model;
	   	byte			wind_speed_index;
   		byte			wind_speeds_size;
  	 	float			wind_gust;
  	 	float			wind_speed;
  	 	float			*wind_speeds;
   		int16_t			wind_direction;
   		uint16_t		bps;
		uint8_t			anemometer_cmd[8];
		uint8_t			answer[7];
		uint8_t			wind_vane_cmd[8];
		uint32_t		polling_ms_interval;
		SoftwareSerial	*sensor_bus;

		void uint64_t_to_uint8_t_array( uint64_t cmd, uint8_t *cmd_array );
};

#endif
