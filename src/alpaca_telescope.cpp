/*	
  	alpaca_telescope.cpp
  	
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

#include <Ethernet.h>
#include <SSLClient.h>
#include <time.h>
#include <AsyncUDP_ESP32_W5500.hpp>
#include <ESPAsyncWebSrv.h>
#include <TinyGPSPlus.h>
#include <ArduinoJson.h>
#include <SiderealPlanets.h>

#include "defaults.h"
#include "gpio_config.h"
#include "SC16IS750.h"
#include "AWSGPS.h"
#include "common.h"
#include "device.h"
#include "dome.h"
#include "alpaca_device.h"
#include "alpaca_server.h"
#include "alpaca_telescope.h"
#include "SQM.h"
#include "config_manager.h"
#include "config_server.h"
#include "device.h"
#include "Hydreon.h"
#include "anemometer.h"
#include "wind_vane.h"
#include "sensor_manager.h"
#include "AstroWeatherStation.h"

extern AstroWeatherStation station;

alpaca_telescope::alpaca_telescope( void ) : alpaca_device( TELESCOPE_INTERFACE_VERSION )
{
	astro_lib.begin();
}

void alpaca_telescope::siderealtime( AsyncWebServerRequest *request, const char *transaction_details )
{
	double		longitude;
	double		latitude;

	if ( get_is_connected() ) {

		if ( !station.get_sensor_data()->gps.fix && !station.is_ntp_synced() )

			snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":%d,"ErrorMessage":"%s",%s})json", 0x500 + static_cast<byte>( ascom_driver_error::NotAvailable ), "No GPS fix and not NTP synced", transaction_details );

		else {
			
			if ( station.get_location_coordinates( &latitude, &longitude )) {
				
				astro_lib.setLatLong( latitude, longitude );
				struct tm dummy;
				struct tm  *utc_time = gmtime_r( &station.get_sensor_data()->gps.time.tv_sec, &dummy );
				utc_time->tm_isdst = -1;
				const struct tm *local_time = localtime_r( &station.get_sensor_data()->gps.time.tv_sec, &dummy );
				time_t time2 = mktime( utc_time );
				astro_lib.setTimeZone( (int)( station.get_sensor_data()->gps.time.tv_sec - time2 ) / 3600 );
				Serial.printf("TZ OFFSET=%d\n", (int)( station.get_sensor_data()->gps.time.tv_sec - time2 ) / 3600);
				if ( local_time->tm_isdst == 1 )
					astro_lib.setDST();
				else
					astro_lib.rejectDST();
				astro_lib.setGMTdate( 1900+utc_time->tm_year, 1+utc_time->tm_mon, utc_time->tm_mday );
				astro_lib.setLocalTime( utc_time->tm_hour, utc_time->tm_min, utc_time->tm_sec );
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%f,%s})json", astro_lib.getLocalSiderealTime(), transaction_details );

			} else

				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":%d,"ErrorMessage":"%s",%s})json", 0x500 + static_cast<byte>( ascom_driver_error::NotAvailable ), "No GPS fix", transaction_details );
		}
	} else

		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Telescope is not connected",%s})json", transaction_details );
	
	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_telescope::siteelevation( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( get_is_connected() ) {

		if ( forced_altitude >= 0 )

			snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%f,%s})json", forced_altitude, transaction_details );

		else {
			
			if ( !station.get_sensor_data()->gps.fix )

				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":%d,"ErrorMessage":"%s",%s})json", 0x500 + static_cast<byte>( ascom_driver_error::NotAvailable ), "No GPS fix", transaction_details );
			else
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%f,%s})json", station.get_sensor_data()->gps.altitude, transaction_details );
		}
	} else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Telescope is not connected",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_telescope::sitelatitude( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( get_is_connected() ) {

		if ( forced_latitude >= 0 )

			snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%f,%s})json", forced_latitude, transaction_details );

		else {

			if ( !station.get_sensor_data()->gps.fix )

				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":%d,"ErrorMessage":"%s",%s})json", 0x500 + static_cast<byte>( ascom_driver_error::NotAvailable ), "No GPS fix", transaction_details );
			else
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%f,%s})json", station.get_sensor_data()->gps.latitude, transaction_details );
		}
	} else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Telescope is not connected",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_telescope::sitelongitude( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( get_is_connected() ) {

		if ( forced_longitude >= 0 )

			snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%f,%s})json", forced_longitude, transaction_details );

		else {

			if ( !station.get_sensor_data()->gps.fix )

				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":%d,"ErrorMessage":"%s",%s})json", 0x500 + static_cast<byte>( ascom_driver_error::NotAvailable ), "No GPS fix", transaction_details );
			else
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.6f,%s})json", station.get_sensor_data()->gps.longitude, transaction_details );

		}
	} else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Telescope is not connected",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_telescope::utcdate( AsyncWebServerRequest *request, const char *transaction_details )
{
	time_t		now;
	if ( get_is_connected() ) {

		if ( !station.get_sensor_data()->gps.fix && !station.is_ntp_synced() )

			snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":%d,"ErrorMessage":"%s",%s})json", 0x500 + static_cast<byte>( ascom_driver_error::NotAvailable ), "No GPS fix and not NTP synced", transaction_details );

		else {
			now = time( nullptr );
			struct tm	dummy;
			struct tm 	*utc_time = gmtime_r( &now, &dummy );
			// flawfinder: ignore
			char tmp[64];
			strftime( tmp, 63, "%FT%TZ", utc_time );
			snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":"%s",%s})json", tmp, transaction_details );
		}

	} else

		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Telescope is not connected",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_telescope::axisrates( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( get_is_connected() )
		snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":"","Value":[{"Maximum":0.0,"Minimum":0.0}]})json", transaction_details );
	else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Telescope is not connected",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_telescope::trackingrates( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( get_is_connected() )
		snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":"","Value":[0]})json", transaction_details );
	else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Telescope is not connected",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_telescope::set_connected( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( request->hasParam( "Connected", true ) ) {

		if ( !strcasecmp( request->getParam( "Connected", true )->value().c_str(), "true" )) {

			if ( station.is_ntp_synced() || station.has_gps() ) {

				set_is_connected( true );
				snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details );

			} else {
					
				set_is_connected( false );
				snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":1031,"ErrorMessage":"No GPS or not NTP synced"})json", transaction_details );
					
			}
				
		} else {

			if ( !strcasecmp( request->getParam( "Connected", true )->value().c_str(), "false" )) {

				set_is_connected( false );
				snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details );

			} else

				snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":1025,"ErrorMessage":"Invalid value (%s)"})json", transaction_details, request->getParam( "Connected", true )->value().c_str() );
		}
		request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
		return;
	}

	request->send( 400, "text/plain", "Missing Connected parameter" );
}

void alpaca_telescope::set_siteelevation( AsyncWebServerRequest *request, const char *transaction_details )
{
	
	if ( request->hasParam( "SiteElevation", true ) ) {

		char	*s = strdup( request->getParam( "SiteElevation", true )->value().c_str() );
		char	*e;
		double	x = strtof( s, &e );
		if (( *e != '\0' ) || ( x >10000 ) || ( x < -300 ))
			snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":1025,"ErrorMessage":"Invalid value (%s)"})json", transaction_details, request->getParam( "SiteElevation", true )->value().c_str() );

		else {

			forced_altitude = x;
			snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details );
		}
		request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
		free( s );
		return;
	}

	request->send( 400, "text/plain", "Missing SiteElevation parameter" );
}

void alpaca_telescope::set_sitelatitude( AsyncWebServerRequest *request, const char *transaction_details )
{

	if ( request->hasParam( "SiteLatitude", true ) ) {

		char	*s = strdup( request->getParam( "SiteLatitude", true )->value().c_str() );
		char	*e;
		double	x = strtof( s, &e );
		if (( *e != '\0' ) || ( x > 90 ) || ( x < -90 ))
			
			snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":1025,"ErrorMessage":"Invalid value (%s)"})json", transaction_details, request->getParam( "SiteLatitude", true )->value().c_str() );

		else {

			forced_latitude = x;
			snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details );
		}
		request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
		free( s );
		return;
	}

	request->send( 400, "text/plain", "Missing SiteLatitude parameter" );
}

void alpaca_telescope::set_sitelongitude( AsyncWebServerRequest *request, const char *transaction_details )
{
	char *s = nullptr;
	char *e;

	if ( request->hasParam( "SiteLongitude", true ) ) {

		s = strdup( request->getParam( "SiteLongitude", true )->value().c_str() );
		double	x = strtof( s, &e );
		if (( *e != '\0' ) || ( x > 180 ) || ( x < -180 ))
			
			snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":1025,"ErrorMessage":"Invalid value (%s)"})json", transaction_details, request->getParam( "SiteLongitude", true )->value().c_str() );

		else {

			forced_longitude = x;
			snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details );
		}
		request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
		free( s );
		return;
	}

	request->send( 400, "text/plain", "Missing SiteLongitude parameter" );
}

void alpaca_telescope::set_utcdate( AsyncWebServerRequest *request, const char *transaction_details )
{
	struct tm		utc_date;
	struct timeval 	now;
	
	if ( request->hasParam( "UTCDate", true ) ) {

		if ( !strptime( request->getParam( "UTCDate", true )->value().c_str(), "%FT%T.", &utc_date )) {

			snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":1025,"ErrorMessage":"Invalid datetime (%s)"})json", transaction_details, request->getParam( "UTCDate", true )->value().c_str() );

		} else {

			now.tv_sec = mktime( &utc_date );
			settimeofday( &now, NULL );
			snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details );

		}
		request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
		return;
	}

	request->send( 400, "text/plain", "Missing UTCDate parameter" );
}
