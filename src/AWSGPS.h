#pragma once
#ifndef _AWSGPS_H
#define _AWSGPS_H

#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>
#include "SC16IS750.h"

struct gps_data_t {

	bool			fix;
	double			longitude;
	double			latitude;
	float			altitude;
	struct timeval	time;

};

class AWSGPS
{
	private:
		TaskHandle_t		gps_task_handle;
		TinyGPSPlus			gps;
#ifdef DEFAULT_HAS_SC16IS750
		I2C_SC16IS750		*gps_serial;
		SemaphoreHandle_t	i2c_mutex = NULL;
#else
		SoftwareSerial		*gps_serial;
#endif
		gps_data_t			*gps_data;
		bool				debug_mode,
							update_rtc;
		void update_data( void );
		void feed( void * );
		
	public:
		AWSGPS( bool );
		bool initialise( gps_data_t *, I2C_SC16IS750 *, SemaphoreHandle_t );
		void resume( void );
		void pilot_rtc( bool );
		bool start( void );
		void stop( void );
		void suspend( void );
};

#endif
