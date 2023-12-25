#pragma once
#ifndef _AWS_WIND_H
#define	_AWS_WIND_H

#include <SoftwareSerial.h>

#define SEND	HIGH
#define RECV	LOW

#define WIND_SENSOR_MAX_TRIES		3		// Tests have shown that if there is no valid answer after 3 attempts, the sensor is not available

class AWSWindSensor {

	public:

				AWSWindSensor( uint32_t, bool );
		bool	anemometer_initialised( void );
		float	get_wind_gust( void );
		bool	initialise_anemometer( byte );
		bool	initialise_wind_vane( byte );
		float	read_anemometer( void );
		int16_t	read_wind_vane( void );
		bool	wind_vane_initialised( void );
		
	private:

   		bool			_anemometer_initialised,
   						debug_mode,
   						_wind_vane_initialised;
	   	byte			anemometer_model,
	   					wind_speed_index,
   						wind_speeds_size;
  	 	float			wind_gust,
  	 					wind_speed,
  	 					*wind_speeds;
   		int16_t			wind_direction;
   		uint16_t		bps;
		uint8_t			anemometer_cmd[8],
						answer[7],
						wind_vane_cmd[8];
		uint32_t		polling_ms_interval;
		SoftwareSerial	*sensor_bus;

		void uint64_t_to_uint8_t_array( uint64_t cmd, uint8_t *cmd_array );
};

#endif
