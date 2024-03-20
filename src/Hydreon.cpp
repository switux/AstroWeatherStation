/*
  	Hydreon.cpp

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

// Keep these two to get rid of compile time errors because of incompatibilities between libraries
#include <AsyncUDP_ESP32_W5500.hpp>
#include <ESPAsyncWebSrv.h>
#include <HardwareSerial.h>
#include <esp_task_wdt.h>

#include "device.h"
#include "Hydreon.h"
#include "common.h"
#include "AstroWeatherStation.h"

RTC_DATA_ATTR uint16_t rain_sensor_baud = 0;	// NOSONAR

const std::array<float,8>	Hydreon::RAIN_RATES	= {
															0.F,			// No rain
															0.1F,			// Rain drops
															1.0F,			// Very light
															2.0F,			// Light
															2.5F,			// Medium light
															5.0F,			// Medium
															10.0F,			// Heavy
															50.1F			// Violent
														};
const std::array<uint16_t,7>	Hydreon::BPS 		= { 1200, 2400, 4800, 9600, 19200, 38400, 57600 };

extern AstroWeatherStation	station;

Hydreon::Hydreon( void ) : rg9_read_mutex( xSemaphoreCreateMutex() )
{
	set_name( "Hydreon RG-9" );
	set_description( "The Hydreon RG-9 Solid State Rain Sensor is a rainfall sensing device." );
	set_driver_version( "1.0" );
	str.clear();
}

void Hydreon::probe( uint16_t baudrate )
{
	status = RAIN_SENSOR_FAIL;

	sensor.begin( baudrate, SERIAL_8N1, rx_pin, tx_pin );
	sensor.println();
	sensor.println();

	if ( get_debug_mode() ) {

		Serial.printf( " @ %dbps: got [", baudrate );
		Serial.flush();
	}

	sensor.println( "B" );
	read_string();

	if ( get_debug_mode() )
		Serial.printf( "%s] ", str.data() );

	if ( !strncmp( str.data(), "Baud ", 5 )) {

		Serial.printf( "%s[HYDREON   ] [INFO ] Found rain sensor @ %d bps\n", get_debug_mode()?"\n":"", baudrate );
		status = RAIN_SENSOR_OK;
		rain_sensor_baud = baudrate;

	} else if ( !strncmp( str.data(), "Reset " , 6 )) {

		status = str[6];
		Serial.printf( "%s[HYDREON   ] [INFO ] Found rain sensor @ %dbps after it was reset because of '%s'\n[INFO ] Rain sensor boot message:\n", get_debug_mode()?"\n":"", baudrate, reset_cause() );

		while ( read_string() )
			Serial.printf( "%s", str.data() );

		rain_sensor_baud = baudrate;
	}
}

bool Hydreon::initialise( void )
{
	if ( rain_sensor_baud ) {

		if ( get_debug_mode() )
			Serial.printf( "[HYDREON   ] [DEBUG] Probing rain sensor using previous birate " );
		probe( rain_sensor_baud );

	} else
		try_baudrates();

	if ( status == RAIN_SENSOR_FAIL ) {

		Serial.printf( "[HYDREON   ] [ERROR] Could not find rain sensor, resetting.\n" );
		pinMode( reset_pin, OUTPUT );
		digitalWrite( reset_pin, LOW );
		delay( 500 );
		digitalWrite( reset_pin, HIGH );
	}

	switch( status ) {

		case RAIN_SENSOR_OK:
			set_initialised( true );
			break;
		case RAIN_SENSOR_FAIL:
			set_initialised( false );
			break;
		case 'B':
			set_initialised( true );
			station.send_alarm( "Rain sensor low voltage", "" );
			break;
		case 'O':
			set_initialised( true );
			station.send_alarm( "Rain sensor problem", "Reset because of stack overflow, report problem to support." );
			break;
		case 'U':
			set_initialised( true );
			station.send_alarm( "Rain sensor problem", "Reset because of stack underflow, report problem to support." );
			break;
		default:
			Serial.printf( "[HYDREON   ] [INFO ] Unhandled rain sensor reset code: %d. Report to support.\n", status );
			set_initialised( false );
			break;
	}
	return get_initialised();
}

bool Hydreon::initialise( HardwareSerial &bus, bool b )
{
	set_debug_mode( b );
	sensor = bus;
	return initialise();
}

byte Hydreon::get_rain_intensity( void )
{
	if ( !get_initialised() && !initialise() ) {

		Serial.printf( "[HYDREON   ] [ERROR] Cannot initialise rain sensor. Not returning rain data.\n" );
		return -1;
	}

	sensor.println( "R" );
	read_string();
	intensity = static_cast<byte>( str[2] - '0' );

	if ( intensity > 7 )	// Most probably during initialisation phase
		intensity = 0;

	if ( get_debug_mode() )
		Serial.printf( "[HYDREON   ] [DEBUG] Rain sensor status string = [%s] intensity=[%d]\n", str.data(), intensity );

	return intensity;
}

const char *Hydreon::get_rain_intensity_str( void )
{
	// As described in RG-9 protocol
	switch( intensity ) {

		case 0: return "No rain";
		case 1: return "Rain drops";
		case 2: return "Very light";
		case 3: return "Medium light";
		case 4: return "Medium";
		case 5: return "Medium heavy";
		case 6: return "Heavy";
		case 7: return "Violent";
	}
	return "Unknown";
}

float Hydreon::get_rain_rate( void )
{
	return Hydreon::RAIN_RATES[ intensity ];
}

byte Hydreon::read_string( void )
{
	str.clear();
	delay( 500 );

	if ( sensor.available() > 0 ) {

		uint8_t i = sensor.readBytes( str.data(), str.capacity() );
		if ( i >= 2 )
			str[ i-2 ] = 0;	// trim trailing \n

		return i-1;
	}

	return 0;
}

const char *Hydreon::reset_cause()
{
	switch ( status ) {

		case 'N': return "Normal power up";
		case 'M': return "MCLR";
		case 'W': return "Watchdog timer reset";
		case 'O': return "Start overflow";
		case 'U': return "Start underflow";
		case 'B': return "Low voltage";
		case 'D': return "Other";
		default: return "Unknown";

	}
}

void Hydreon::try_baudrates( void )
{
	for ( byte i = 0; i < HYDREON_PROBE_RETRIES; i++ ) {

		if ( get_debug_mode() )
			Serial.printf( "[HYDREON   ] [DEBUG] Probing rain sensor, attempt #%d: ...", i );

		for ( byte j = 0; j < BPS.size(); j++ ) {

			esp_task_wdt_reset();
			probe( BPS[j] );
			if ( status == RAIN_SENSOR_FAIL )
				sensor.end();
			else
				return;
		}

		if ( status != RAIN_SENSOR_FAIL )
				return;
	}
}
