/*	
  	Hydreon.cpp
  	
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

#include <esp_task_wdt.h>
#include <Arduino.h>
#include <HardwareSerial.h>
#include "Hydreon.h"

RTC_DATA_ATTR uint16_t baud=0;

static constexpr int bps[ HYDREON_SERIAL_SPEEDS ] = { 1200, 2400, 4800, 9600, 19200, 38400, 57600 };

Hydreon::Hydreon( uint8_t _uart_nr, uint8_t tx, uint8_t rx, uint8_t reset, bool _debug_mode )
{
	initialised = false;
	uart_nr = _uart_nr;
	rx_pin = rx;
	tx_pin = tx;
	reset_pin = reset;
	debug_mode = _debug_mode;
	status = ' ';
}

void Hydreon::probe( uint16_t baudrate )
{
	int l;
	char bitrate[18]={0};
	
	status = RAIN_SENSOR_FAIL;
	
	sensor->begin( baudrate, SERIAL_8N1, rx_pin, tx_pin );
	sensor->println();
	sensor->println();

	if ( debug_mode ) {

		Serial.printf( " @ %dbps: got [", baudrate );
		Serial.flush();
	}

	sensor->println( "B" );
	l = read_string();

	if ( debug_mode )
		Serial.printf( "%s] ", str );

	if ( !strncmp( str, "Baud ", 5 )) {

		Serial.printf( "%s[INFO] Found rain sensor @ %d bps\n", debug_mode?"\n":"", baudrate );
		status = RAIN_SENSOR_OK;
		baud = baudrate;

	} else if ( !strncmp( str, "Reset " , 6 )) {

		status = str[6];
		Serial.printf( "%s[INFO] Found rain sensor @ %dbps after it was reset because of '%s'\n[INFO] Rain sensor boot message:\n", debug_mode?"\n":"", baudrate, reset_cause() );

		while (( l = read_string()))
			Serial.println( str );

		baud = baudrate;
	}
}

bool Hydreon::initialise( void )
{
	sensor = new HardwareSerial( uart_nr );

	if ( baud ) {

		if ( debug_mode )
			Serial.printf( "[DEBUG] Probing rain sensor using previous birate " );
		probe( baud );

	} else 
		for ( byte i = 0; i < HYDREON_PROBE_RETRIES; i++ ) {

			if ( debug_mode )
				Serial.printf( "[DEBUG] Probing rain sensor, attempt #%d: ...", i );
			
			for ( byte j = 0; j < HYDREON_SERIAL_SPEEDS; j++ ) {

				esp_task_wdt_reset();
				probe( bps[j] );
				if ( status == RAIN_SENSOR_FAIL )
					sensor->end();
				else 
					break;
			}
			
			if ( debug_mode )
				printf( "\n" );

			if ( status != RAIN_SENSOR_FAIL )
				break;
		}

	if ( status == RAIN_SENSOR_FAIL ) {

		Serial.printf( "[ERROR] Could not find rain sensor, resetting.\n" );
		pinMode( reset_pin, OUTPUT );
		digitalWrite( reset_pin, LOW );
		delay( 500 );
		digitalWrite( reset_pin, HIGH );
	}

	// FIXME: restore alarms
	switch( status ) {
	
		case RAIN_SENSOR_OK:
			initialised = true;
			break;
		case RAIN_SENSOR_FAIL:
			initialised = false;
			break;
		case 'B':
			initialised = true;
//			send_alarm( runtime_config, "Rain sensor low voltage", "", debug_mode );
			break;
		case 'O':
			initialised = true;
	//		send_alarm( runtime_config, "Rain sensor problem", "Reset because of stack overflow, report problem to support.", debug_mode );
			break;
		case 'U':
			initialised = true;
		//	send_alarm( runtime_config, "Rain sensor problem", "Reset because of stack underflow, report problem to support.", debug_mode );
			break;
		default:
			Serial.printf( "[INFO] Unhandled rain sensor reset code: %d. Report to support.\n", status );
			initialised = false;
			break;
	}
	return initialised;
}

byte Hydreon::rain_intensity( void  )
{
	if (( !initialised ) && ( !initialise() )) {

		Serial.printf( "[ERROR] Cannot initialise rain sensor. Not returning rain data.\n" );
		return -1;
	}
		
	sensor->println( "R" );
	memset( str, 0, 128 );
	read_string();

	if ( debug_mode )
		Serial.printf( "[DEBUG] Rain sensor status string = [%s]\n", str );

	return static_cast<byte>( str[2] - '0' );
}

uint8_t Hydreon::read_string( void )
{
	uint8_t i;
	
	//FIXME: check if really needed
	delay( 500 );

	if ( sensor->available() > 0 ) {

		i = sensor->readBytes( str, 128 );
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
