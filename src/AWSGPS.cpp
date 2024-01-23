/*	
  	AWSGPS.cpp
  	
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

#include "defaults.h"
#include "gpio_config.h"
#include "AWSGPS.h"

AWSGPS::AWSGPS( bool _debug_mode ) : gps_task_handle( NULL )
{
	debug_mode = _debug_mode;
	update_rtc = false;
	sc16is750 = NULL;
	gps_serial = NULL;
	gps_data = NULL;
}

void AWSGPS::read_GPS( void )
{
	unsigned long _start = millis();

	if ( sc16is750 ) {

		do {
			while( sc16is750->available() )
				// flawfinder: ignore
				gps.encode( sc16is750->read() );
			delay( 5 );
		} while( ( millis() - _start ) < 1000 );

	} else {

		do {
			while( gps_serial->available() )
				// flawfinder: ignore
				gps.encode( gps_serial->read() );
			delay( 5 );
		} while( ( millis() - _start ) < 1000 );
	}
}

void AWSGPS::feed( void *dummy )
{
	while( true ) {

		if ( sc16is750 ) {

			if ( xSemaphoreTake( i2c_mutex, 500 / portTICK_PERIOD_MS ) == pdTRUE ) {

				read_GPS();
				xSemaphoreGive( i2c_mutex );

			} else
				delay( 5000 );

		} else
			read_GPS();

		update_data();
		delay( 5000 );
	}
}

void AWSGPS::update_data( void )
{
	struct tm gpstime = {0};
	time_t now;
	
	gps_data->fix = gps.location.isValid();

	if ( gps_data->fix ) {

		gps_data->longitude = gps.location.lng();
		gps_data->latitude = gps.location.lat();
		gps_data->altitude = gps.altitude.meters();
		gpstime.tm_year = 	gps.date.year() - 1900;
		gpstime.tm_mon = gps.date.month() - 1;
		gpstime.tm_mday = gps.date.day();
		gpstime.tm_hour = gps.time.hour();
		gpstime.tm_min = gps.time.minute();
		gpstime.tm_sec = gps.time.second();
		gpstime.tm_isdst = -1;
		now = mktime( &gpstime );
		gps_data->time.tv_sec = now;
		if ( update_rtc )
			settimeofday( &gps_data->time, 0 );

	} else {

		gps_data->longitude = 0.F;
		gps_data->latitude = 0.F;
		gps_data->altitude = 0.F;
		now = mktime( &gpstime );
		gps_data->time.tv_sec = now;

	}
}

bool AWSGPS::initialise( gps_data_t *_gps_data, I2C_SC16IS750 *_sc16is750, SemaphoreHandle_t _i2c_mutex )
{
	gps_data = _gps_data;

	i2c_mutex = _i2c_mutex;
	while ( xSemaphoreTake( i2c_mutex, 500 / portTICK_PERIOD_MS ) != pdTRUE );
	sc16is750 = _sc16is750;

	if ( !sc16is750->begin( GPS_SPEED )) {

		Serial.printf( "[ERROR] Could not find I2C UART. GPS disabled!\n" );
		xSemaphoreGive( i2c_mutex );
		return false;
	}
	xSemaphoreGive( i2c_mutex );
	return true;
}

bool AWSGPS::initialise( gps_data_t *_gps_data )
{
	gps_data = _gps_data;

	gps_serial = new HardwareSerial(2);
	gps_serial->begin( GPS_SPEED, SERIAL_8N1, GPS_RX, GPS_TX );
	return ( gps_serial->availableForWrite() > 0 );
}

void AWSGPS::pilot_rtc( bool _update_rtc )
{
	update_rtc = _update_rtc;
}

void AWSGPS::resume( void )
{
	if ( gps_task_handle != NULL )
		vTaskResume( gps_task_handle );
}

bool AWSGPS::start( void )
{
	std::function<void(void *)> _feed = std::bind( &AWSGPS::feed, this, std::placeholders::_1 );
	xTaskCreatePinnedToCore( 
		[](void *param) {
            std::function<void(void*)>* feed_proxy = static_cast<std::function<void(void*)>*>( param );
            (*feed_proxy)( NULL );
		}, "GPSFeed", 2000, &_feed, 4, &gps_task_handle, 1 );

	return true;
}

void AWSGPS::suspend( void )
{
	if ( gps_task_handle != NULL )
		vTaskSuspend( gps_task_handle );
}

void AWSGPS::stop( void )
{
	if ( gps_task_handle != NULL )
		vTaskDelete( gps_task_handle );

	gps_task_handle = NULL;
}
