/*
  	AWSGPS.h

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
#ifndef _AWSGPS_H
#define _AWSGPS_H

#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>
#include "SC16IS750.h"

struct gps_data_t {

	bool			fix;
	float			longitude;
	float			latitude;
	float			altitude;
	struct timeval	time;

};

class AWSGPS
{
	private:

		TaskHandle_t		gps_task_handle;
		TinyGPSPlus			gps;
		I2C_SC16IS750		*sc16is750		= nullptr;
		SemaphoreHandle_t	i2c_mutex		= nullptr;
		HardwareSerial		*gps_serial		= nullptr;
		gps_data_t			*gps_data		= nullptr;
		bool				debug_mode		= false;
		bool				update_rtc		= false;
	
		void update_data( void );
		void feed( void * );
		void read_GPS( void );

	public:

					AWSGPS( void ) = default;
		bool		initialise( gps_data_t * );
		bool		initialise( gps_data_t *, I2C_SC16IS750 *, SemaphoreHandle_t );
		void		resume( void );
		void		pilot_rtc( bool );
		bool		start( void );
		void		stop( void );
		void		suspend( void );
};

#endif
