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
#include <ESPAsyncWebServer.h>
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

void alpaca_telescope::axisrates( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( get_is_connected() ) {

		const char *param = has_parameter( request, "Axis", false );

		if ( param != nullptr ) {

			char	*e;
			float	x = strtof( request->getParam( param, false )->value().c_str(), &e );

			if ( *e != '\0' ) {

				request->send( 400, "text/plain", "Bad parameter value" );
				return;
			}
		
			if ( snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":"","Value":[{"Maximum":0.0,"Minimum":0.0}]})json", transaction_details.data() ) < 0 ) {

					request->send( 500, "text/plain", "Failed to format answer" );
					return;
			}
			
		} else {

			request->send( 400, "text/plain", "Missing parameter" );
			return;
		}

	} else {

		if ( snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Telescope is not connected",%s})json", transaction_details.data() ) < 0 ) {

				request->send( 500, "text/plain", "Failed to format answer" );
				return;
		}
	}
	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_telescope::canmoveaxis( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( get_is_connected() ) {

		const char *param = has_parameter( request, "Axis", false );

		if ( param != nullptr ) {

			char	*e;
			float	x = strtof( request->getParam( param, false )->value().c_str(), &e );

			if ( *e != '\0' ) {

				request->send( 400, "text/plain", "Bad parameter value" );
				return;
			}
		
			send_value( request, transaction_details, false );
			return;

		} else {

			request->send( 400, "text/plain", "Missing parameter" );
			return;
		}

	} else {

		if ( snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Telescope is not connected",%s})json", transaction_details.data() ) < 0 ) {

				request->send( 500, "text/plain", "Failed to format answer" );
				return;
		}
	}
}

void alpaca_telescope::siderealtime( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	float		longitude;
	float		latitude;

	if ( get_is_connected() ) {

		if ( !station.get_station_data()->gps.fix && !station.is_ntp_synced() ) {

			if ( snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":%d,"ErrorMessage":"%s",%s})json", 0x500 + static_cast<byte>( ascom_driver_error::NotAvailable ), "No GPS fix and not NTP synced", transaction_details.data() ) < 0 ) {

				request->send( 500, "text/plain", "Failed to format answer" );
				return;
			}

		} else {

			if ( station.get_location_coordinates( &latitude, &longitude )) {

				astro_lib.setLatLong( latitude, longitude );
				struct tm dummy;
				struct tm  *utc_time = gmtime_r( &station.get_station_data()->gps.time.tv_sec, &dummy );
				utc_time->tm_isdst = -1;
				const struct tm *local_time = localtime_r( &station.get_station_data()->gps.time.tv_sec, &dummy );
				time_t time2 = mktime( utc_time );
				astro_lib.setTimeZone( (int)( station.get_station_data()->gps.time.tv_sec - time2 ) / 3600 );
				if ( local_time->tm_isdst == 1 )
					astro_lib.setDST();
				else
					astro_lib.rejectDST();
				astro_lib.setGMTdate( 1900+utc_time->tm_year, 1+utc_time->tm_mon, utc_time->tm_mday );
				astro_lib.setLocalTime( utc_time->tm_hour, utc_time->tm_min, utc_time->tm_sec );
				if ( snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%f,%s})json", astro_lib.getLocalSiderealTime(), transaction_details.data() ) < 0 ) {

					request->send( 500, "text/plain", "Failed to format answer" );
					return;
				}

			} else {

				if ( snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":%d,"ErrorMessage":"%s",%s})json", 0x500 + static_cast<byte>( ascom_driver_error::NotAvailable ), "No GPS fix", transaction_details.data() ) < 0 ) {

					request->send( 500, "text/plain", "Failed to format answer" );
					return;
				}
			}
		}

	} else {

		if ( snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Telescope is not connected",%s})json", transaction_details.data() ) < 0 ) {

			request->send( 500, "text/plain", "Failed to format answer" );
			return;
		}
	}

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_telescope::siteelevation( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( get_is_connected() ) {

		if ( forced_altitude >= 0 ) {

			if ( snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%f,%s})json", forced_altitude, transaction_details.data() ) < 0 ) {

				request->send( 500, "text/plain", "Failed to format answer" );
				return;
			}

		} else {

			if ( !station.get_station_data()->gps.fix ) {

				if ( snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":%d,"ErrorMessage":"%s",%s})json", 0x500 + static_cast<byte>( ascom_driver_error::NotAvailable ), "No GPS fix", transaction_details.data() ) < 0 ) {

					request->send( 500, "text/plain", "Failed to format answer" );
					return;
				}

			} else {

				if ( snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%f,%s})json", station.get_station_data()->gps.altitude, transaction_details.data() ) < 0 ) {

					request->send( 500, "text/plain", "Failed to format answer" );
					return;
				}
			}
		}

	} else {

		if ( snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Telescope is not connected",%s})json", transaction_details.data() ) < 0 ) {

				request->send( 500, "text/plain", "Failed to format answer" );
				return;
		}
	}
	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_telescope::sitelatitude( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( get_is_connected() ) {

		if ( forced_latitude >= 0 ) {

			if ( snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%f,%s})json", forced_latitude, transaction_details.data() ) < 0 ) {

				request->send( 500, "text/plain", "Failed to format answer" );
				return;
			}

		} else {

			if ( !station.get_station_data()->gps.fix ) {

				if ( snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":%d,"ErrorMessage":"%s",%s})json", 0x500 + static_cast<byte>( ascom_driver_error::NotAvailable ), "No GPS fix", transaction_details.data() ) < 0 ) {

					request->send( 500, "text/plain", "Failed to format answer" );
					return;
				}

			} else {

				if ( snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%f,%s})json", station.get_station_data()->gps.latitude, transaction_details.data() ) < 0 ) {

					request->send( 500, "text/plain", "Failed to format answer" );
					return;
				}
			}
		}

	} else {

		if ( snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Telescope is not connected",%s})json", transaction_details.data() ) < 0 ) {

				request->send( 500, "text/plain", "Failed to format answer" );
				return;
		}
	}
	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_telescope::sitelongitude( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( get_is_connected() ) {

		if ( forced_longitude >= 0 ) {

			if ( snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%f,%s})json", forced_longitude, transaction_details.data() ) < 0 ) {

				request->send( 500, "text/plain", "Failed to format answer" );
				return;
			}

		} else {

			if ( !station.get_station_data()->gps.fix ) {

				if ( snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":%d,"ErrorMessage":"%s",%s})json", 0x500 + static_cast<byte>( ascom_driver_error::NotAvailable ), "No GPS fix", transaction_details.data() ) < 0 ) {

					request->send( 500, "text/plain", "Failed to format answer" );
					return;
				}

			} else {

				if ( snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.6f,%s})json", station.get_station_data()->gps.longitude, transaction_details.data() ) < 0 ) {

					request->send( 500, "text/plain", "Failed to format answer" );
					return;
				}
			}
		}

	} else {

		if ( snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Telescope is not connected",%s})json", transaction_details.data() ) < 0 ) {

				request->send( 500, "text/plain", "Failed to format answer" );
				return;
		}
	}

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_telescope::utcdate( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	time_t		now;

	if ( get_is_connected() ) {

		if ( !station.get_station_data()->gps.fix && !station.is_ntp_synced() ) {

			if ( snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":%d,"ErrorMessage":"%s",%s})json", 0x500 + static_cast<byte>( ascom_driver_error::NotAvailable ), "No GPS fix and not NTP synced", transaction_details.data() ) < 0 ) {

				request->send( 500, "text/plain", "Failed to format answer" );
				return;
			}

		} else {
			now = time( nullptr );
			struct tm	dummy;
			struct tm 	*utc_time = gmtime_r( &now, &dummy );
			etl::string<64> tmp;
			strftime( tmp.data(), tmp.capacity(), "%FT%TZ", utc_time );
			if ( snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":"%s",%s})json", tmp.data(), transaction_details.data() ) < 0 ) {

				request->send( 500, "text/plain", "Failed to format answer" );
				return;
			}
		}

	} else {

		if ( snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Telescope is not connected",%s})json", transaction_details.data() ) < 0 ) {

				request->send( 500, "text/plain", "Failed to format answer" );
				return;
		}
	}
	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_telescope::trackingrates( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( get_is_connected() ) {

		if ( snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":"","Value":[0]})json", transaction_details.data() ) < 0 ) {

				request->send( 500, "text/plain", "Failed to format answer" );
				return;
		}

	} else {

		if ( snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Telescope is not connected",%s})json", transaction_details.data() ) < 0 ) {

				request->send( 500, "text/plain", "Failed to format answer" );
				return;
		}
	}
	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_telescope::set_connected( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( request->hasParam( "Connected", true ) ) {

		if ( !strcasecmp( request->getParam( "Connected", true )->value().c_str(), "true" )) {

			if ( station.is_ntp_synced() || station.has_device( aws_device_t::GPS_SENSOR ) ) {

				set_is_connected( true );
				if ( snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details.data() ) < 0 ) {

					request->send( 500, "text/plain", "Failed to format answer" );
					return;
				}

			} else {

				set_is_connected( false );
				if ( snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":1031,"ErrorMessage":"No GPS or not NTP synced"})json", transaction_details.data() ) < 0 ) {

					request->send( 500, "text/plain", "Failed to format answer" );
					return;
				}
			}

		} else {

			if ( !strcasecmp( request->getParam( "Connected", true )->value().c_str(), "false" )) {

				set_is_connected( false );
				if ( snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details.data() ) < 0 ) {

					request->send( 500, "text/plain", "Failed to format answer" );
					return;
				}

			} else

				if ( snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":1025,"ErrorMessage":"Invalid value (%s)"})json", transaction_details.data(), request->getParam( "Connected", true )->value().c_str() ) < 0 ) {

					request->send( 500, "text/plain", "Failed to format answer" );
					return;
				}
		}

		request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
		return;
	}

	request->send( 400, "text/plain", "Missing Connected parameter" );
}

void alpaca_telescope::set_siteelevation( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{

	const char *param = has_parameter( request, "SiteElevation", false );

	if ( param != nullptr ) {

		char	*e;
		float	x = strtof( request->getParam( param, true )->value().c_str(), &e );

		if (( *e != '\0' ) || ( x >10000 ) || ( x < -300 )) {

			if ( snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":1025,"ErrorMessage":"Invalid value (%s)"})json", transaction_details.data(), request->getParam( "SiteElevation", true )->value().c_str() ) < 0 ) {

				request->send( 500, "text/plain", "Failed to format answer" );
				return;
			}

		} else {

			forced_altitude = x;
			if ( snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details.data() ) < 0 ) {

				request->send( 500, "text/plain", "Failed to format answer" );
				return;
			}
		}
		request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
		return;
	}

	request->send( 400, "text/plain", "Missing SiteElevation parameter" );
}

void alpaca_telescope::set_sitelatitude( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	const char *param = has_parameter( request, "SiteLatitude", false );

	if ( param != nullptr ) {

		char	*e;
		float	x = strtof( request->getParam( param, true )->value().c_str(), &e );
		if (( *e != '\0' ) || ( x > 90 ) || ( x < -90 )) {

			if ( snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":1025,"ErrorMessage":"Invalid value (%s)"})json", transaction_details.data(), request->getParam( "SiteLatitude", true )->value().c_str() ) < 0 ) {

				request->send( 500, "text/plain", "Failed to format answer" );
				return;
			}

		} else {

			forced_latitude = x;
			if ( snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details.data() ) < 0 ) {

				request->send( 500, "text/plain", "Failed to format answer" );
				return;
			}
		}
		request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
		return;
	}

	request->send( 400, "text/plain", "Missing SiteLatitude parameter" );
}

void alpaca_telescope::set_sitelongitude( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	const char *param = has_parameter( request, "SiteLongitude", false );

	if ( param != nullptr ) {

		char	*e;
		float	x = strtof( request->getParam( param, true )->value().c_str(), &e );

		if (( *e != '\0' ) || ( x > 180 ) || ( x < -180 )) {

			if ( snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":1025,"ErrorMessage":"Invalid value (%s)"})json", transaction_details.data(), request->getParam( "SiteLongitude", true )->value().c_str() ) < 0 ) {

				request->send( 500, "text/plain", "Failed to format answer" );
				return;
			}

		} else {

			forced_longitude = x;
			if ( snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details.data() ) < 0 ) {

				request->send( 500, "text/plain", "Failed to format answer" );
				return;
			}
		}
		request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
		return;
	}

	request->send( 400, "text/plain", "Missing SiteLongitude parameter" );
}

void alpaca_telescope::set_utcdate( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	struct tm		utc_date;
	struct timeval 	now;

	const char *param = has_parameter( request, "UTCDate", false );

	if ( param != nullptr ) {

		if ( !strptime( request->getParam( param, true )->value().c_str(), "%FT%T.", &utc_date )) {

			if ( snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":1025,"ErrorMessage":"Invalid datetime (%s)"})json", transaction_details.data(), request->getParam( "UTCDate", true )->value().c_str() ) < 0 ) {

				request->send( 500, "text/plain", "Failed to format answer" );
				return;
			}

		} else {

			now.tv_sec = mktime( &utc_date );
			settimeofday( &now, NULL );
			if ( snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details.data() ) < 0 ) {

				request->send( 500, "text/plain", "Failed to format answer" );
				return;
			}
		}
		request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
		return;
	}

	request->send( 400, "text/plain", "Missing UTCDate parameter" );
}
