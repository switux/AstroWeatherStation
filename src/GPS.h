#pragma once
#ifndef _AWSGPS_H
#define _AWSGPS_H

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
		TaskHandle_t	gps_task_handle;
		TinyGPSPlus		gps;
#ifdef SC16IS750
		I2C_SC16IS750	*gps_serial;
#else
		SoftwareSerial	*gps_serial;
#endif
		gps_data_t		*gps_data;
		bool			debug_mode;

		void update_data( void );
		void feed( void * );
		
	public:
		AWSGPS( bool );
		bool initialise( gps_data_t * );
		void resume( void );
		bool start( void );
		void stop( void );
		void suspend( void );
};

#endif
