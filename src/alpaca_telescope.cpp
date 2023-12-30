/*	
  	alpaca_telescope.cpp
  	
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
#include "AstroWeatherStation.h"
#include "dome.h"
#include "alpaca.h"
#include "alpaca_telescope.h"
#include "SQM.h"
#include "AWSConfig.h"
#include "AWSWeb.h"
#include "Hydreon.h"
#include "AWSWind.h"
#include "AWSSensorManager.h"
#include "AWS.h"

extern AstroWeatherStation station;

alpaca_telescope::alpaca_telescope( bool _debug_mode )
{
	is_connected = false;
	debug_mode = _debug_mode;
	devicetype = "Telescope";
	_supportedactions = TELESCOPE_SUPPORTED_ACTIONS;
	_driverinfo = TELESCOPE_DRIVER_INFO;
	_name = TELESCOPE_NAME;
	_description = TELESCOPE_DESCRIPTION;
	_driverversion = TELESCOPE_DRIVER_VERSION;
	_interfaceversion = TELESCOPE_INTERFACE_VERSION;
	astro_lib.begin();
}

void alpaca_telescope::siderealtime( AsyncWebServerRequest *request, const char *transaction_details )
{
	double		longitude,
				latitude;
	struct tm 	*utc_time,
				*local_time;
	time_t		time2;

	if ( is_connected ) {

		if ( !station.get_sensor_data()->gps.fix && !station.is_ntp_synced() )

			snprintf( (char *)message_str, 255, "{\"ErrorNumber\":%d,\"ErrorMessage\":\"%s\",%s}", 0x500 + NotAvailable, "No GPS fix and not NTP synced", transaction_details );

		else {
			
			if ( station.get_location_coordinates( &latitude, &longitude )) {
				
				astro_lib.setLatLong( latitude, longitude );
				utc_time = gmtime( &station.get_sensor_data()->gps.time.tv_sec );
				utc_time->tm_isdst = -1;
				local_time = localtime( &station.get_sensor_data()->gps.time.tv_sec );
				time2 = mktime( utc_time );
				astro_lib.setTimeZone( (int)( station.get_sensor_data()->gps.time.tv_sec - time2 ) / 3600 );
				Serial.printf("TZ OFFSET=%d\n", (int)( station.get_sensor_data()->gps.time.tv_sec - time2 ) / 3600);
				if ( local_time->tm_isdst == 1 )
					astro_lib.setDST();
				else
					astro_lib.rejectDST();
				astro_lib.setGMTdate( 1900+utc_time->tm_year, 1+utc_time->tm_mon, utc_time->tm_mday );
				astro_lib.setLocalTime( utc_time->tm_hour, utc_time->tm_min, utc_time->tm_sec );
				snprintf( (char *)message_str, 255, "{\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":%f,%s}", astro_lib.getLocalSiderealTime(), transaction_details );

			} else

				snprintf( (char *)message_str, 255, "{\"ErrorNumber\":%d,\"ErrorMessage\":\"%s\",%s}", 0x500 + NotAvailable, "No GPS fix", transaction_details );
		}
	} else

		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":1031,\"ErrorMessage\":\"Dome is not connected\",%s}", transaction_details );
	
	request->send( 200, "application/json", (const char*)message_str );
}

void alpaca_telescope::siteelevation( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected ) {

		if ( forced_altitude >= 0 )

			snprintf( (char *)message_str, 255, "{\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":%f,%s}", forced_altitude, transaction_details );

		else {
			
			if ( !station.get_sensor_data()->gps.fix )

				snprintf( (char *)message_str, 255, "{\"ErrorNumber\":%d,\"ErrorMessage\":\"%s\",%s}", 0x500 + NotAvailable, "No GPS fix", transaction_details );
			else
				snprintf( (char *)message_str, 255, "{\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":%f,%s}", station.get_sensor_data()->gps.altitude, transaction_details );
		}
	} else
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":1031,\"ErrorMessage\":\"Dome is not connected\",%s}", transaction_details );

	request->send( 200, "application/json", (const char*)message_str );
}

void alpaca_telescope::sitelatitude( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected ) {

		if ( forced_latitude >= 0 )

			snprintf( (char *)message_str, 255, "{\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":%f,%s}", forced_latitude, transaction_details );

		else {

			if ( !station.get_sensor_data()->gps.fix )

				snprintf( (char *)message_str, 255, "{\"ErrorNumber\":%d,\"ErrorMessage\":\"%s\",%s}", 0x500 + NotAvailable, "No GPS fix", transaction_details );
			else
				snprintf( (char *)message_str, 255, "{\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":%f,%s}", station.get_sensor_data()->gps.latitude, transaction_details );
		}
	} else
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":1031,\"ErrorMessage\":\"Dome is not connected\",%s}", transaction_details );

	request->send( 200, "application/json", (const char*)message_str );
}

void alpaca_telescope::sitelongitude( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected ) {

		if ( forced_longitude >= 0 )

			snprintf( (char *)message_str, 255, "{\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":%f,%s}", forced_longitude, transaction_details );

		else {

			if ( !station.get_sensor_data()->gps.fix )

				snprintf( (char *)message_str, 255, "{\"ErrorNumber\":%d,\"ErrorMessage\":\"%s\",%s}", 0x500 + NotAvailable, "No GPS fix", transaction_details );
			else
				snprintf( (char *)message_str, 255, "{\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":%3.6f,%s}", station.get_sensor_data()->gps.longitude, transaction_details );

		}
	} else
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":1031,\"ErrorMessage\":\"Dome is not connected\",%s}", transaction_details );

	request->send( 200, "application/json", (const char*)message_str );
}

void alpaca_telescope::utcdate( AsyncWebServerRequest *request, const char *transaction_details )
{
	char		tmp[64];
	struct tm 	*utc_time;
	time_t		now;
	if ( is_connected ) {

		if ( !station.get_sensor_data()->gps.fix && !station.is_ntp_synced() )

			snprintf( (char *)message_str, 255, "{\"ErrorNumber\":%d,\"ErrorMessage\":\"%s\",%s}", 0x500 + NotAvailable, "No GPS fix and not NTP synced", transaction_details );

		else {
			now = time( NULL );
			utc_time = gmtime( &now );
			strftime( tmp, 63, "%FT%TZ", utc_time );
			snprintf( (char *)message_str, 255, "{\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":\"%s\",%s}", tmp, transaction_details );
		}		

	} else
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":1031,\"ErrorMessage\":\"Dome is not connected\",%s}", transaction_details );

	request->send( 200, "application/json", (const char*)message_str );
}

void alpaca_telescope::axisrates( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected )
		snprintf( (char *)message_str, 255, "{%s,\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":[{\"Maximum\":0.0,\"Minimum\":0.0}]}", transaction_details );
	else
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":1031,\"ErrorMessage\":\"Dome is not connected\",%s}", transaction_details );

	request->send( 200, "application/json", (const char*)message_str );
}

void alpaca_telescope::trackingrates( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected )
		snprintf( (char *)message_str, 255, "{%s,\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":[0]}", transaction_details );
	else
		snprintf( (char *)message_str, 255, "{\"ErrorNumber\":1031,\"ErrorMessage\":\"Dome is not connected\",%s}", transaction_details );

	request->send( 200, "application/json", (const char*)message_str );
}

void alpaca_telescope::set_connected( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( request->hasParam( "Connected", true ) ) {

		if ( !strcasecmp( request->getParam( "Connected", true )->value().c_str(), "true" )) {

			if ( station.is_ntp_synced() || station.has_gps() ) {

				is_connected = true;
				snprintf( (char *)message_str, 255, "{%s,\"ErrorNumber\":0,\"ErrorMessage\":\"\"}", transaction_details );

			} else {
					
				is_connected = false;
				snprintf( (char *)message_str, 255, "{%s,\"ErrorNumber\":1031,\"ErrorMessage\":\"No GPS or not NTP synced\"}", transaction_details );
					
			}
				
		} else {

			if ( !strcasecmp( request->getParam( "Connected", true )->value().c_str(), "false" )) {

				is_connected = false;
				snprintf( (char *)message_str, 255, "{%s,\"ErrorNumber\":0,\"ErrorMessage\":\"\"}", transaction_details );

			} else

				snprintf( (char *)message_str, 255, "{%s,\"ErrorNumber\":1025,\"ErrorMessage\":\"Invalid value (%s)\"}", transaction_details, request->getParam( "Connected", true )->value().c_str() );
		}
		request->send( 200, "application/json", (const char*)message_str );
		return;
	}

	request->send( 400, "text/plain", "Missing Connected parameter" );
}

void alpaca_telescope::set_siteelevation( AsyncWebServerRequest *request, const char *transaction_details )
{
	double x;
	
	if ( request->hasParam( "SiteElevation", true ) ) {

		x = atof( request->getParam( "SiteElevation", true )->value().c_str() );
		if (( x == 0 ) || ( x >10000 ) || ( x < -300 ))
			snprintf( (char *)message_str, 255, "{%s,\"ErrorNumber\":1025,\"ErrorMessage\":\"Invalid value (%s)\"}", transaction_details, request->getParam( "SiteElevation", true )->value().c_str() );

		else {

			forced_altitude = x;
			snprintf( (char *)message_str, 255, "{%s,\"ErrorNumber\":0,\"ErrorMessage\":\"\"}", transaction_details );
		}
		request->send( 200, "application/json", (const char*)message_str );
		return;
	}

	request->send( 400, "text/plain", "Missing SiteElevation parameter" );
}

void alpaca_telescope::set_sitelatitude( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( request->hasParam( "SiteLatitude", true ) ) {

		double x = atof( request->getParam( "SiteLatitude", true )->value().c_str() );
		if (( x == 0 ) || ( x > 90 ) || ( x < -90 ))
			
			snprintf( (char *)message_str, 255, "{%s,\"ErrorNumber\":1025,\"ErrorMessage\":\"Invalid value (%s)\"}", transaction_details, request->getParam( "SiteLatitude", true )->value().c_str() );

		else {

			forced_latitude = x;
			snprintf( (char *)message_str, 255, "{%s,\"ErrorNumber\":0,\"ErrorMessage\":\"\"}", transaction_details );
		}
		request->send( 200, "application/json", (const char*)message_str );
		return;
	}

	request->send( 400, "text/plain", "Missing SiteLatitude parameter" );
}

void alpaca_telescope::set_sitelongitude( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( request->hasParam( "SiteLongitude", true ) ) {

		double x = atof( request->getParam( "SiteLongitude", true )->value().c_str() );
		if (( x == 0 ) || ( x > 180 ) || ( x < -180 ))
			
			snprintf( (char *)message_str, 255, "{%s,\"ErrorNumber\":1025,\"ErrorMessage\":\"Invalid value (%s)\"}", transaction_details, request->getParam( "SiteLongitude", true )->value().c_str() );

		else {

			forced_longitude = x;
			snprintf( (char *)message_str, 255, "{%s,\"ErrorNumber\":0,\"ErrorMessage\":\"\"}", transaction_details );
		}
		request->send( 200, "application/json", (const char*)message_str );
		return;
	}

	request->send( 400, "text/plain", "Missing SiteLongitude parameter" );
}

void alpaca_telescope::set_utcdate( AsyncWebServerRequest *request, const char *transaction_details )
{
	char			*tmp;
	struct tm		utc_date;
	struct timeval 	now;
	
	if ( request->hasParam( "UTCDate", true ) ) {

		if ( !(tmp = strptime( request->getParam( "UTCDate", true )->value().c_str(), "%FT%T.", &utc_date ))) {

			snprintf( (char *)message_str, 255, "{%s,\"ErrorNumber\":1025,\"ErrorMessage\":\"Invalid datetime (%s)\"}", transaction_details, request->getParam( "UTCDate", true )->value().c_str() );

		} else {

			now.tv_sec = mktime( &utc_date );
			settimeofday( &now, NULL );
			snprintf( (char *)message_str, 255, "{%s,\"ErrorNumber\":0,\"ErrorMessage\":\"\"}", transaction_details );

		}
		request->send( 200, "application/json", (const char*)message_str );
		return;
	}

	request->send( 400, "text/plain", "Missing UTCDate parameter" );
}
