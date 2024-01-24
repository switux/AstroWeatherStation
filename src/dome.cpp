/*
  	dome.cpp

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

#include <Arduino.h>

#include "gpio_config.h"
#include "SC16IS750.h"
#include "dome.h"

AWSDome::AWSDome( bool _debug_mode )
{
	debug_mode = _debug_mode;
	sc16is750 = NULL;

	if ( debug_mode )
		Serial.printf( "[DEBUG] Enabling dome control.\n" );

	pinMode( GPIO_DOME_1, OUTPUT );
	pinMode( GPIO_DOME_2, OUTPUT );
	pinMode( GPIO_DOME_STATUS, INPUT );

	start_control_task();
}

AWSDome::AWSDome( I2C_SC16IS750 *_sc16is750, SemaphoreHandle_t _i2c_mutex, bool _debug_mode ) : i2c_mutex( _i2c_mutex )
{
	debug_mode = _debug_mode;
//	i2c_mutex = _i2c_mutex;
	sc16is750 = _sc16is750;
	close_dome = false;

	if ( debug_mode )
		Serial.printf( "[DEBUG] Enabling dome control through SC16IS750.\n" );

	while ( xSemaphoreTake( i2c_mutex, 500 / portTICK_PERIOD_MS ) != pdTRUE );

	if ( !sc16is750->begin( 9600 )) {		// FIXME: should not need to set baud rate to only use GPIO

		Serial.printf( "[ERROR] Could not find I2C UART. Cannot control dome!\n" );
		xSemaphoreGive( i2c_mutex );
		return;
	}

	sc16is750->pinMode( GPIO_DOME_1, OUTPUT );
	sc16is750->pinMode( GPIO_DOME_2, OUTPUT );
	xSemaphoreGive( i2c_mutex );

	pinMode( GPIO_DOME_STATUS, INPUT );

	start_control_task();
}

void AWSDome::close( void *dummy )
{
	while( true ) {

		if ( close_dome ) {

			Serial.printf( "[INFO] Closing dome.\n" );
			if ( sc16is750 ) {

				while ( xSemaphoreTake( i2c_mutex, 50 / portTICK_PERIOD_MS ) != pdTRUE );

				sc16is750->digitalWrite( GPIO_DOME_1, HIGH );
				sc16is750->digitalWrite( GPIO_DOME_2, HIGH );
				delay( 2000 );
				sc16is750->digitalWrite( GPIO_DOME_1, LOW );
				sc16is750->digitalWrite( GPIO_DOME_2, LOW );

				xSemaphoreGive( i2c_mutex );

			} else {

				digitalWrite( GPIO_DOME_1, HIGH );
				digitalWrite( GPIO_DOME_2, HIGH );
				delay( 2000 );
				digitalWrite( GPIO_DOME_1, LOW );
				digitalWrite( GPIO_DOME_2, LOW );

			}
			close_dome = false;
		}
		delay( 1000 );
	}
}

bool AWSDome::closed( void )
{
	bool x = ( digitalRead( GPIO_DOME_STATUS ) == LOW );

	if ( debug_mode )
		Serial.printf( "[DEBUG] Dome status: %s\n", x ? "closed" : "open" );

	return x;
}

void AWSDome::start_control_task( void )
{
	// Controlling the dome from an ISR just does not work because the wdt fires off
	// during I2C communication with the SC16IS750
	// Instead we just toggle a flag which is then read by a high prio task in charge of
	// closing the dome.

	std::function<void(void *)> _close = std::bind( &AWSDome::close, this, std::placeholders::_1 );
	xTaskCreatePinnedToCore(
		[](void *param) {
            std::function<void(void*)>* close_proxy = static_cast<std::function<void(void*)>*>( param );
            (*close_proxy)( NULL );
		}, "DomeControl", 2000, &_close, configMAX_PRIORITIES - 2, &dome_task_handle, 1 );
}

void AWSDome::trigger_close( void )
{
	close_dome = true;
}
