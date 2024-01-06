/*	
  	Hydreon.h
  	
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
#ifndef _HYDREON_H
#define _HYDREON_H

#define HYDREON_SERIAL_SPEEDS	7
#define HYDREON_PROBE_RETRIES	3

#define RAIN_SENSOR_OK			0
#define RAIN_SENSOR_FAIL		127
#define RAIN_SENSOR_UART		2

#include <Arduino.h>

class Hydreon {

	private:

		bool			debug_mode,
						initialised;
		uint8_t			uart_nr,
						intensity,
						reset_pin,
						rx_pin,
						tx_pin;
		char			str[128],
						status;
				
		HardwareSerial	*sensor;
		SemaphoreHandle_t rg9_read_mutex;

		void			probe( uint16_t );
		byte			read_string( void );

	public:

					Hydreon( uint8_t, uint8_t, uint8_t, uint8_t, bool );
		bool 		initialise( void );
		byte		rain_intensity( void  );
		const char	*rain_intensity_str( void );
		const char	*reset_cause( void );

};

#endif
