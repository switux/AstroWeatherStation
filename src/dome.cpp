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
#include "device.h"
#include "dome.h"

Dome::Dome( void )
{
	set_name( "Roll-off roof" );
	set_description( "Generic relay driven roll-off roof" );
	set_driver_version( "1.0" );
}

void Dome::close( void *dummy ) // NOSONAR
{
	while( true ) {

		if ( close_dome ) {

			Serial.printf( "[INFO] Closing dome.\n" );
			if ( sc16is750 ) {

				while ( xSemaphoreTake( i2c_mutex, 50 / portTICK_PERIOD_MS ) != pdTRUE );

				sc16is750->digitalWrite( GPIO_DOME_1, LOW );
				sc16is750->digitalWrite( GPIO_DOME_2, LOW );

				xSemaphoreGive( i2c_mutex );

			} else {

				digitalWrite( GPIO_DOME_1, LOW );
				digitalWrite( GPIO_DOME_2, LOW );

			}
			close_dome = false;
		}
		delay( 1000 );
	}
}

bool Dome::closed( void )
{
	bool x = ( digitalRead( GPIO_DOME_STATUS ) == LOW );

	if ( get_debug_mode() )
		Serial.printf( "[DEBUG] Dome status: %s\n", x ? "closed" : "open" );

	return x;
}

void Dome::initialise( bool _debug_mode )
{
	set_debug_mode( _debug_mode );
	
	// Controlling the dome from an ISR just does not work because the wdt fires off
	// during I2C communication with the SC16IS750
	// Instead we just toggle a flag which is then read by a high prio task in charge of
	// closing the dome.

	std::function<void(void *)> _close = std::bind( &Dome::close, this, std::placeholders::_1 );
	xTaskCreatePinnedToCore(
		[](void *param) {	// NOSONAR
            std::function<void(void*)>* close_proxy = static_cast<std::function<void(void*)>*>( param );
            (*close_proxy)( NULL );
		}, "DomeControl", 2000, &_close, configMAX_PRIORITIES - 2, &dome_task_handle, 1 );
}

void Dome::initialise( I2C_SC16IS750 *_sc16is750, SemaphoreHandle_t _i2c_mutex, bool _debug_mode )
{
	i2c_mutex =_i2c_mutex;
	sc16is750 = _sc16is750;
	initialise( _debug_mode );
}

void Dome::trigger_close( void )
{
  close_dome = true;
}
