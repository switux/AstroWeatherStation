#include <Arduino.h>
#include <esp_task_wdt.h>

#include "common.h"
#include "SC16IS750.h"
#include "dome.h"
#include "config_manager.h"
#include "sensor_manager.h"
#include "AWSLookout.h"

void AWSLookout::loop( void * )	// NOSONAR
{
	while( true ) {
		Serial.printf( "LOOKOUT LOOP\n" );
		delay( 1000 );	
	}
}

void AWSLookout::initialise( AWSConfig *_config, AWSSensorManager *_mngr, Dome *_dome, bool _debug_mode )
{
	debug_mode = _debug_mode;
	dome = _dome;
	sensor_manager = _mngr;
	config = _config;
	
	std::function<void(void *)> _loop = std::bind( &AWSLookout::loop, this, std::placeholders::_1 );
	xTaskCreatePinnedToCore(
		[](void *param) {	// NOSONAR
			std::function<void(void*)>* periodic_tasks_proxy = static_cast<std::function<void(void*)>*>( param );	// NOSONAR
			(*periodic_tasks_proxy)( NULL );
		}, "AWSLookout Task", 4000, &_loop, 5, &watcher_task_handle, 1 );

}
