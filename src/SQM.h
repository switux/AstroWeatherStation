#pragma once
#ifndef _SQM_H
#define _SQM_H

#undef CONFIG_DISABLE_HAL_LOCKS
#define _ASYNC_WEBSERVER_LOGLEVEL_       0
#define _ETHERNET_WEBSERVER_LOGLEVEL_      0

#define	DOWN	-1
#define	UP		1

#include "Adafruit_TSL2591.h"

class SQM {

	public:
		SQM( Adafruit_TSL2591 *, sensor_data_t * );
		void read_SQM( float );
		void set_debug_mode( bool );
		void set_msas_calibration_offset( float );
		
	private:
		Adafruit_TSL2591 *tsl;
		bool debug_mode;
		float msas_calibration_offset;
		sensor_data_t *sensor_data;
		
		float ch0_temperature_factor( float );
		float ch1_temperature_factor( float );
		void change_gain( uint8_t, tsl2591Gain_t * );
		void change_integration_time( uint8_t, tsl2591IntegrationTime_t * );
		bool SQM_get_msas_nelm(  float, float *, float *, uint16_t *, uint16_t *, uint16_t *, uint16_t * );
		uint8_t SQM_read_with_extended_integration_time( float, uint16_t *, uint16_t *, uint16_t * );
		
};

#endif
