/*
  	SQM.cpp

  	(c) 2023-2024 F.Lesage

	1.0.0 - Initial version, derived from AWS 2.0

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

#include <Arduino.h>
#include <esp_task_wdt.h>

#include "common.h"
#include "device.h"
#include "SQM.h"
#include "Hydreon.h"
#include "sensor_manager.h"

void SQM::initialise( Adafruit_TSL2591 *_tsl, sqm_data_t *data, float calibration_offset, bool _debug_mode )
{
	tsl = _tsl;
	sqm_data = data;
	msas_calibration_offset = calibration_offset;
	debug_mode = _debug_mode;
}

void SQM::set_msas_calibration_offset( float _msas_calibration_offset )
{
	msas_calibration_offset = _msas_calibration_offset;
}

//
// Formulas inferred from the TSL2591 datasheet fig.15, page 9. Data points graphically extracted (yuck!).
// Thanks to Marco Gulino <marco.gulino@gmail.com> for pointing this graph to me!
// I favoured a power function over an affine because it better followed the points on the graph
// It may however be overkill since the points come from a PDF graph: the source error is certainly not negligible :-)
//

float SQM::ch0_temperature_factor( float temp )
{
	if ( temp < 0 )
		return 1.F;
	return 0.9759F + 0.00192947F*pow( temp, 0.783129F );
}

float SQM::ch1_temperature_factor( float temp )
{
	if ( temp < 0 )
		return 1.F;
	return 1.05118F - 0.0023342F*pow( temp, 0.958056F );
}

void SQM::change_gain( uint8_t upDown, tsl2591Gain_t *gain_idx )
{
	tsl2591Gain_t	g = static_cast<tsl2591Gain_t>( *gain_idx + 16*upDown );

	if ( g < TSL2591_GAIN_LOW )
		g = TSL2591_GAIN_LOW;
	else if ( g > TSL2591_GAIN_MAX )
		g = TSL2591_GAIN_MAX;

	if ( g != *gain_idx ) {

		tsl->setGain( g );
		*gain_idx = g;
	}
}

void SQM::change_integration_time( uint8_t upDown, tsl2591IntegrationTime_t *int_time_idx )
{
	tsl2591IntegrationTime_t	t = static_cast<tsl2591IntegrationTime_t>( *int_time_idx + 2*upDown );

	if ( t < TSL2591_INTEGRATIONTIME_100MS )
		t = TSL2591_INTEGRATIONTIME_100MS;
	else if ( t > TSL2591_INTEGRATIONTIME_600MS )
		t = TSL2591_INTEGRATIONTIME_600MS;

	if ( t != *int_time_idx ) {

		tsl->setTiming( t );
		*int_time_idx = t;
	}
}

void SQM::read( float ambient_temp )
{
	tsl->setGain( TSL2591_GAIN_LOW );
	tsl->setTiming( TSL2591_INTEGRATIONTIME_100MS );

	while ( !get_msas_nelm( ambient_temp ));
}

bool SQM::decrease_gain( tsl2591Gain_t *gain_idx )
{
	if ( *gain_idx != TSL2591_GAIN_LOW ) {

		if ( debug_mode )
			Serial.printf( "[SQM       ] [DEBUG] Decreasing gain.\n" );
		change_gain( DOWN, gain_idx );
		return false;

	}
	return true;
}

bool SQM::decrease_integration_time( tsl2591IntegrationTime_t *int_time_idx )
{
	if ( *int_time_idx != TSL2591_INTEGRATIONTIME_100MS ) {

		if ( debug_mode )
			Serial.printf( "[SQM       ] [DEBUG] Decreasing integration time.\n" );
		change_integration_time( DOWN, int_time_idx );
		return true;
	}
	return false;	
}

bool SQM::increase_gain( tsl2591Gain_t *gain_idx )
{
	if ( *gain_idx != TSL2591_GAIN_MAX ) {

		if ( debug_mode )
			Serial.printf( "[SQM       ] [DEBUG] Increasing gain.\n" );
		change_gain( UP, gain_idx );
		return true;
	}
	return false;
}

bool SQM::increase_integration_time( tsl2591IntegrationTime_t *int_time_idx )
{
	if ( *int_time_idx != TSL2591_INTEGRATIONTIME_600MS ) {

		if ( debug_mode )
			Serial.printf( "[SQM       ] [DEBUG] Increasing integration time.\n" );
		change_integration_time( UP, int_time_idx );
		return true;
	}
	return false;
}

bool SQM::get_msas_nelm( float ambient_temp )
{
	uint32_t	both_channels;
	uint16_t	ir_luminosity;
	uint16_t	full_luminosity;
	uint16_t	visible_luminosity;
	uint8_t		iterations = 1;

	tsl2591Gain_t				gain_idx;
	tsl2591IntegrationTime_t	int_time_idx;

	const std::array<uint16_t,4>	gain_factor			= { 1, 25, 428, 9876 };
	const std::array<uint16_t,6>	integration_time	= { 100, 200, 300, 400, 500, 600 };

	esp_task_wdt_reset();

	gain_idx = tsl->getGain();
	int_time_idx = tsl->getTiming();
	both_channels = tsl->getFullLuminosity();
	ir_luminosity = static_cast<uint16_t>( both_channels >> 16 );
	full_luminosity = static_cast<uint16_t>( both_channels & 0xFFFF );
	ir_luminosity = static_cast<uint16_t>( static_cast<float>(ir_luminosity) * ch1_temperature_factor( ambient_temp ) );
	full_luminosity = static_cast<uint16_t>( static_cast<float>(full_luminosity) * ch0_temperature_factor( ambient_temp ) );
	visible_luminosity = full_luminosity - ir_luminosity;

	// On some occasions this can happen, leading to high values of "visible" although it is dark (as the variable is unsigned), giving erroneous msas
	if ( full_luminosity < ir_luminosity )
		return false;

	if ( debug_mode )
		Serial.printf( "[SQM       ] [DEBUG] gain=0x%02x (%dx) time=0x%02x (%dms)/ temp=%2.2f° / Infrared=%05d Full=%05d Visible=%05d\n", gain_idx, gain_factor[ gain_idx >> 4 ], int_time_idx, integration_time[ int_time_idx ], ambient_temp, ir_luminosity, full_luminosity, visible_luminosity );

	// Auto gain and integration time, increase time before gain to avoid increasing noise if we can help it, decrease gain first for the same reason
	// Then retry by returning false
	if ( visible_luminosity < 128 ) {

		if ( increase_integration_time( &int_time_idx ))
			return false;

		if ( increase_gain( &gain_idx ))
			return false;

		iterations = read_with_extended_integration_time( ambient_temp, &ir_luminosity, &full_luminosity, &visible_luminosity );
		if ( full_luminosity < ir_luminosity )
			return false;

	}

	if (( full_luminosity == 0xFFFF ) || ( ir_luminosity == 0xFFFF )) {

		if ( decrease_gain( &gain_idx ))
			return false;

		if ( decrease_integration_time( &int_time_idx ))
			return false;

	}

	// Comes from Adafruit TSL2591 driver
	float cpl = static_cast<float>( gain_factor[ gain_idx >> 4 ] ) * static_cast<float>( integration_time[ int_time_idx ] ) / 408.F;
	float lux = ( static_cast<float>(visible_luminosity) * ( 1.F-( static_cast<float>(ir_luminosity)/static_cast<float>(full_luminosity))) ) / cpl;

	// About the MSAS formula, quoting http://unihedron.com/projects/darksky/magconv.php:
	// This formula was derived from conversations on the Yahoo-groups darksky-list
	// Topic: [DSLF]  Conversion from mg/arcsec^2 to cd/m^2
	// Date range: Fri, 1 Jul 2005 17:36:41 +0900 to Fri, 15 Jul 2005 08:17:52 -0400

	// I added a calibration offset to match readings from my SQM-LE
	sqm_data->msas = ( log10( lux / 108000.F ) / -0.4F ) + msas_calibration_offset;
	if ( sqm_data->msas < 0 )
		sqm_data->msas = 0;
	sqm_data->nelm = 7.93F - 5.F * log10( pow( 10, ( 4.316F - ( sqm_data->msas / 5.F ))) + 1.F );

	sqm_data->integration_time = integration_time[ int_time_idx ];
	sqm_data->gain = gain_factor[ gain_idx >> 4 ];
	sqm_data->full_luminosity = full_luminosity;
	sqm_data->ir_luminosity = ir_luminosity;
	if ( debug_mode )
		Serial.printf( "[SQM       ] [DEBUG] GAIN=[0x%02hhx/%ux] TIME=[0x%02hhx/%ums] Iterations=[%d] Visible=[%05d] Infrared=[%05d] MPSAS=[%f] NELM=[%2.2f]\n", gain_idx, gain_factor[ gain_idx >> 4 ], int_time_idx, integration_time[ int_time_idx ], iterations, visible_luminosity, ir_luminosity, sqm_data->msas, sqm_data->nelm );

	return true;
}

uint8_t SQM::read_with_extended_integration_time( float ambient_temp, uint16_t *cumulated_ir, uint16_t *cumulated_full, uint16_t *cumulated_visible )
{
	uint8_t		iterations = 1;

	while (( *cumulated_visible < 128 ) && ( iterations <= 32 )) {

		iterations++;
		uint32_t both_channels = tsl->getFullLuminosity();
		uint16_t _ir_luminosity = both_channels >> 16;
		uint16_t _full_luminosity = both_channels & 0xFFFF;
		_ir_luminosity = static_cast<uint16_t>( static_cast<float>(_ir_luminosity) * ch1_temperature_factor( ambient_temp ));
		_full_luminosity = static_cast<uint16_t>( static_cast<float>(_full_luminosity) * ch0_temperature_factor( ambient_temp ));
		*cumulated_full += _full_luminosity;
		*cumulated_ir += _ir_luminosity;
		*cumulated_visible = *cumulated_full - *cumulated_ir;

		delay( 50 );
	}

	return iterations;
}
