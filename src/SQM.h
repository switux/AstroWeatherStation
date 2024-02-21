/*	
  	SQM.h
  	
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
#ifndef _SQM_H
#define _SQM_H

#define	DOWN	( -1 )
#define	UP		( 1 )

#include "Adafruit_TSL2591.h"

class SQM {

	public:

		SQM( void ) = default;
		void initialise( Adafruit_TSL2591 *, sqm_data_t *, float, bool );
		void read_SQM( float );
		void set_msas_calibration_offset( float );
		
	private:

		bool				debug_mode				= false;
		float				msas_calibration_offset	= 0.F;
		sqm_data_t			*sqm_data			= nullptr;
		Adafruit_TSL2591	*tsl;
		
		float ch0_temperature_factor( float );
		float ch1_temperature_factor( float );
		void change_gain( uint8_t, tsl2591Gain_t * );
		void change_integration_time( uint8_t, tsl2591IntegrationTime_t * );
		bool SQM_get_msas_nelm(  float, float *, float *, uint16_t *, uint16_t *, uint16_t *, uint16_t * );
		uint8_t SQM_read_with_extended_integration_time( float, uint16_t *, uint16_t *, uint16_t * );
		
};

#endif
