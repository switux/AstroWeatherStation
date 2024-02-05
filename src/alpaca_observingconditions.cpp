/*
  	alpaca_observingconditions.cpp

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

#include <AsyncUDP_ESP32_W5500.hpp>
#include <ESPAsyncWebSrv.h>

#include "defaults.h"
#include "gpio_config.h"
#include "common.h"
#include "alpaca_device.h"
#include "alpaca_server.h"
#include "alpaca_observingconditions.h"
#include "AstroWeatherStation.h"

extern AstroWeatherStation station;

constexpr unsigned int str2int( const char* str, int h = 0 )
{
    return !str[h] ? 5381 : (str2int(str, h+1) * 33) ^ str[h];
}

alpaca_observingconditions::alpaca_observingconditions( bool _debug_mode )
{
	is_connected = false;
	debug_mode = _debug_mode;
	devicetype = "Sensor";
	_supportedactions = OBSERVINGCONDITIONS_SUPPORTED_ACTIONS;
	_driverinfo = OBSERVINGCONDITIONS_DRIVER_INFO;
	_name = OBSERVINGCONDITIONS_NAME;
	_description = OBSERVINGCONDITIONS_DESCRIPTION;
	_driverversion = OBSERVINGCONDITIONS_DRIVER_VERSION;
	_interfaceversion = OBSERVINGCONDITIONS_INTERFACE_VERSION;
}

void alpaca_observingconditions::cloudcover( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected && station.is_sensor_initialised( MLX_SENSOR ))
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.1f,%s})json", station.get_sensor_data()->cloud_cover, transaction_details );
	else
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1024,"ErrorMessage":"Sensor is not available",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_observingconditions::dewpoint( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected && station.is_sensor_initialised( BME_SENSOR ))
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%2.1f,%s})json", station.get_sensor_data()->dew_point, transaction_details );
	else
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1024,"ErrorMessage":"Sensor is not available",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_observingconditions::get_averageperiod( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected )
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":0.0,%s})json", transaction_details );
	else
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1024,"ErrorMessage":"Sensor is not available",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_observingconditions::humidity( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected && station.is_sensor_initialised( BME_SENSOR ))
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.1f,%s})json", station.get_sensor_data()->rh, transaction_details );
	else
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1024,"ErrorMessage":"Sensor is not available",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_observingconditions::pressure( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected && station.is_sensor_initialised( BME_SENSOR ))
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%4.1f,%s})json", station.get_sensor_data()->pressure, transaction_details );
	else
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1024,"ErrorMessage":"Sensor is not available",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_observingconditions::rainrate( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( !station.has_rain_sensor() )

		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":%d,"ErrorMessage":"The station has no rain sensor",%s})json", 1023 + static_cast<byte>( ascom_error::PropertyOrMethodNotImplemented ), transaction_details );

	else {

		if ( is_connected && station.is_sensor_initialised( RAIN_SENSOR )) {

			short x = station.get_sensor_data()->rain_intensity;

			if ( x >= 0 )
				snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.1f,%s})json", rain_rate[ x ], transaction_details );
			else
				snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":%d,"ErrorMessage":"Rain sensor data is temporarily unavailable",%s})json", 1023 + static_cast<byte>( ascom_error::PropertyOrMethodNotImplemented ), transaction_details );

		} else

			snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1031,"ErrorMessage":"Sensor is not connected",%s})json", transaction_details );
	}

	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_observingconditions::refresh( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected ) {

		if ( station.poll_sensors() )

			snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"",%s})json", transaction_details );

		else {

			request->send( 400, "application/json", "Could not refresh sensors" );
			return;

		}
	} else

		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1031,"ErrorMessage":"Sensor is not connected",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_observingconditions::sensordescription( AsyncWebServerRequest *request, const char *transaction_details )
{

	if ( is_connected ) {

		bool ok = false;
		for( int i = 0; i < request->params(); i++ ) {

			if ( !strcasecmp( request->getParam(i)->name().c_str(), "SensorName" )) {

				// flawfinder: ignore
				char tmp[32];
				int l = strlcpy( tmp, request->getParam(i)->value().c_str(), 31 );
				for( int j = 0; j < l; tmp[j]= tolower( tmp[j] ), j++ );
				ok = true;
				switch( str2int( tmp )) {

					case str2int("pressure"):
					case str2int("temperature"):
					case str2int("humidity"):
					case str2int("dewpoint"):
						if ( station.is_sensor_initialised( BME_SENSOR ))
							snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":"BME280",%s})json", transaction_details );
						else
							snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1024,"ErrorMessage":"%s sensor is not available",%s})json", request->getParam(i)->value().c_str(), transaction_details );
						break;
					case str2int("skybrightness"):
					case str2int("skyquality"):
						if ( station.is_sensor_initialised( TSL_SENSOR ))
							snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":"TSL 2591",%s})json", transaction_details );
						else
							snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1024,"ErrorMessage":"%s sensor is not available",%s})json", request->getParam(i)->value().c_str(), transaction_details );
						break;
					case str2int("cloudcover"):
					case str2int("skytemperature"):
						if ( station.is_sensor_initialised( MLX_SENSOR ))
							snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":"MLX 96014",%s})json", transaction_details );
						else
							snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1024,"ErrorMessage":"%s sensor is not available",%s})json", request->getParam(i)->value().c_str(), transaction_details );
						break;
					case str2int("rainrate"):
						if ( station.is_sensor_initialised( RAIN_SENSOR ))
							snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":"Hydreon RG-9",%s})json", transaction_details );
						else
							snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1024,"ErrorMessage":"%s sensor is not available",%s})json", request->getParam(i)->value().c_str(), transaction_details );
						break;
					case str2int("windspeed"):
					case str2int("windgust"):
						if ( station.is_sensor_initialised( ANEMOMETER_SENSOR ))
							snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":"%s",%s})json", station.get_anemometer_sensorname(), transaction_details );
						else
							snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1024,"ErrorMessage":"%s sensor is not available",%s})json", request->getParam(i)->value().c_str(), transaction_details );
						break;
					case str2int("winddirection"):
						if ( station.is_sensor_initialised( WIND_VANE_SENSOR ))
							snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":"%s",%s})json", station.get_wind_vane_sensorname(), transaction_details );
						else
							snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1024,"ErrorMessage":"%s sensor is not available",%s})json", request->getParam(i)->value().c_str(), transaction_details );
						break;
					case str2int("starfwhm"):
						snprintf( static_cast<char *>( message_str ), 255, R"json({%s,"ErrorNumber":1024,"ErrorMessage":"No such sensor: %s"})json", transaction_details, tmp );
						break;
					default:
						snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1025,"ErrorMessage":"","Value":"No such sensor name",%s})json", transaction_details );
				}
			}
		}
		if ( !ok ) {

			request->send( 400, "application/json", "Missing SensorName parameter key" );
			return;
		}
	} else
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1031,"ErrorMessage":"ObservingConditions device is not connected",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_observingconditions::set_averageperiod( AsyncWebServerRequest *request, const char *transaction_details )
{

	if ( is_connected ) {
			if ( request->hasParam( "AveragePeriod", true ) ) {

				float x = atof( request->getParam( "AveragePeriod", true )->value().c_str() );
				if ( x == 0 )

					snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"",%s})json", transaction_details );

				else {

					if ( x < 0 )
						snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":%d,"ErrorMessage":"Value must be positive or 0",%s})json", 1023 + static_cast<byte>( ascom_error::InvalidValue ), transaction_details );
					else
						snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":%d,"ErrorMessage":"Only providing live data, please set to 0.",%s})json", 1023 + static_cast<byte>( ascom_error::InvalidValue ), transaction_details );

        }
			} else {

				request->send( 400, "text/plain", "Missing AveragePeriod parameter" );
				return;
			}
	}
	else
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1031,"ErrorMessage":"Sensor is not connected",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_observingconditions::set_connected( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( request->hasParam( "Connected", true ) ) {

		if ( !strcasecmp( request->getParam( "Connected", true )->value().c_str(), "true" )) {

			is_connected = true;
			snprintf( static_cast<char *>( message_str ), 255, R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details );

		} else {

			if ( !strcasecmp( request->getParam( "Connected", true )->value().c_str(), "false" )) {

				is_connected = false;
				snprintf( static_cast<char *>( message_str ), 255, R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details );

			} else

				snprintf( static_cast<char *>( message_str ), 255, R"json({%s,"ErrorNumber":1025,"ErrorMessage":"Invalid value %s"})json", transaction_details, request->getParam( "Connected", true )->value().c_str() );
		}
		request->send( 200, "application/json", static_cast<const char *>( message_str ) );
		return;
	}

	request->send( 400, "text/plain", "Missing Connected parameter" );
}

void alpaca_observingconditions::skybrightness( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected && station.is_sensor_initialised( TSL_SENSOR ))
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%6.4f,%s})json", (double)station.get_sensor_data()->lux, transaction_details );
	else
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1024,"ErrorMessage":"Sensor is not available",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_observingconditions::skyquality( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected && station.is_sensor_initialised( TSL_SENSOR ))
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%2.2f,%s})json", station.get_sensor_data()->msas, transaction_details );
	else
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1024,"ErrorMessage":"Sensor is not available",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_observingconditions::skytemperature( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected && station.is_sensor_initialised( MLX_SENSOR ))
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%2.2f,%s})json", station.get_sensor_data()->sky_temperature, transaction_details );
	else
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1024,"ErrorMessage":"Sensor is not available",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_observingconditions::temperature( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected && station.is_sensor_initialised( BME_SENSOR ))
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%2.2f,%s})json", station.get_sensor_data()->temperature, transaction_details );
	else
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1024,"ErrorMessage":"Sensor is not available",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_observingconditions::timesincelastupdate( AsyncWebServerRequest *request, const char *transaction_details )
{
	time_t now;
	time( &now );

	if ( is_connected ) {

		bool ok = false;
		for( int i = 0; i < request->params(); i++ ) {

			if ( !strcasecmp( request->getParam(i)->name().c_str(), "SensorName" )) {

				// flawfinder: ignore
				char tmp[32];
				int l = strlcpy( tmp, request->getParam(i)->value().c_str(), 31 );
				for( int j = 0; j < l; tmp[j]= tolower( tmp[j] ), j++ );
				ok = true;
				switch( str2int( tmp )) {

					case str2int("pressure"):
					case str2int("temperature"):
					case str2int("humidity"):
					case str2int("dewpoint"):
						if ( !station.is_sensor_initialised( BME_SENSOR ))
							snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1024,"ErrorMessage":"%s is not available",%s})json", request->getParam(i)->value().c_str(), transaction_details );
						else
							snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.1f,%s})json", (double)( now - station.get_sensor_data()->timestamp ), transaction_details );
						break;
					case str2int("skybrightness"):
					case str2int("skyquality"):
						if ( !station.is_sensor_initialised( TSL_SENSOR ))
							snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1024,"ErrorMessage":"%s is not available",%s})json", request->getParam(i)->value().c_str(), transaction_details );
						else
							snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.1f,%s})json", (double)( now - station.get_sensor_data()->timestamp ), transaction_details );
						break;
					case str2int("cloudcover"):
					case str2int("skytemperature"):
						if ( !station.is_sensor_initialised( MLX_SENSOR ))
							snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1024,"ErrorMessage":"%s is not available",%s})json", request->getParam(i)->value().c_str(), transaction_details );
						else
							snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.1f,%s})json", (double)( now - station.get_sensor_data()->timestamp ), transaction_details );
						break;
					case str2int("rainrate"):
						if ( !station.is_sensor_initialised( RAIN_SENSOR ))
							snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1024,"ErrorMessage":"%s is not available",%s})json", request->getParam(i)->value().c_str(), transaction_details );
						else
							snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.1f,%s})json", (double)( now - station.get_sensor_data()->timestamp ), transaction_details );
						break;
					case str2int("windspeed"):
					case str2int("windgust"):
						if ( !station.is_sensor_initialised( ANEMOMETER_SENSOR ))
							snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1024,"ErrorMessage":"%s is not available",%s})json", request->getParam(i)->value().c_str(), transaction_details );
						else
							snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.1f,%s})json", (double)( now - station.get_sensor_data()->timestamp ), transaction_details );
						break;
					case str2int("winddirection"):
						if ( !station.is_sensor_initialised( WIND_VANE_SENSOR ))
							snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1024,"ErrorMessage":"%s is not available",%s})json", request->getParam(i)->value().c_str(), transaction_details );
						else
							snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.1f,%s})json", (double)( now - station.get_sensor_data()->timestamp ), transaction_details );
						break;

					case str2int(""):
						if ( station.is_sensor_initialised( WIND_VANE_SENSOR ) ||
							station.is_sensor_initialised( ANEMOMETER_SENSOR ) ||
							station.is_sensor_initialised( RAIN_SENSOR ) ||
							station.is_sensor_initialised( MLX_SENSOR ) ||
							station.is_sensor_initialised( TSL_SENSOR ) ||
							station.is_sensor_initialised( MLX_SENSOR ))
							snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.1f,%s})json", (double)( now - station.get_sensor_data()->timestamp ), transaction_details );
						else
							snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1024,"ErrorMessage":"No sensor is available",%s})json", transaction_details );
						break;
					case str2int("starfwhm"):
						snprintf( static_cast<char *>( message_str ), 255, R"json({%s,"ErrorNumber\":1024,"ErrorMessage":"No such sensor: %s"})json", transaction_details, tmp );
						break;
					default:
						snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1025,"ErrorMessage":"","Value":"No such sensor name: %s",%s})json", request->getParam(i)->value().c_str(), transaction_details );
				}
			}
		}
		if ( !ok ) {

			request->send( 400, "application/json", "Missing SensorName parameter key" );
			return;
		}

	} else
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1031,"ErrorMessage":"Device is not connected",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_observingconditions::winddirection( AsyncWebServerRequest *request, const char *transaction_details )
{
	uint16_t x = station.get_sensor_data()->wind_direction;

	if ( station.get_sensor_data()->wind_speed > 0 )
		x = ( x == 0 ) ? 360 : x;
	else
		x = 0;

	if ( is_connected && station.is_sensor_initialised( WIND_VANE_SENSOR ))
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.1f,%s})json", (double)x, transaction_details );
	else
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1024,"ErrorMessage":"No sensor is available",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_observingconditions::windgust( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected && station.is_sensor_initialised( ANEMOMETER_SENSOR ))
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.1f,%s})json", station.get_sensor_data()->wind_gust, transaction_details );
	else
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1024,"ErrorMessage":"No sensor is available",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}

void alpaca_observingconditions::windspeed( AsyncWebServerRequest *request, const char *transaction_details )
{
	if ( is_connected && station.is_sensor_initialised( ANEMOMETER_SENSOR ))
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.1f,%s})json", station.get_sensor_data()->wind_speed, transaction_details );
	else
		snprintf( static_cast<char *>( message_str ), 255, R"json({"ErrorNumber":1024,"ErrorMessage":"No sensor is available",%s})json", transaction_details );

	request->send( 200, "application/json", static_cast<const char *>( message_str ) );
}
