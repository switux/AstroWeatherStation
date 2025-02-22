/*
  	alpaca_observingconditions.cpp

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

#include <AsyncUDP_ESP32_W5500.hpp>
#include <ESPAsyncWebServer.h>

#include "Embedded_Template_Library.h"
#include "etl/string.h"
#include "etl/string_utilities.h"

#include "defaults.h"
#include "gpio_config.h"
#include "common.h"
#include "alpaca_device.h"
#include "alpaca_server.h"
#include "alpaca_observingconditions.h"
#include "AstroWeatherStation.h"

extern AstroWeatherStation station;

alpaca_observingconditions::alpaca_observingconditions( void ): alpaca_device( OBSERVINGCONDITIONS_INTERFACE_VERSION )
{
}

void alpaca_observingconditions::averageperiod( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( request->method() == HTTP_GET ) {

		if ( get_is_connected() )
			snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":0.0,%s})json", transaction_details.data() );
		else
			snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1024,"ErrorMessage":"Sensor is not available",%s})json", transaction_details.data() );

		request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
		return;
	}

	if ( get_is_connected() ) {

		const char *param = has_parameter( request, "AveragePeriod", false );

		if ( param == nullptr ) {

			request->send( 400, "text/plain", "Missing AveragePeriod parameter" );
			return;
		}

		char	*e;
		float	x = strtof( request->getParam( param, true )->value().c_str(), &e );

		if ( *e != '\0' ) {

			request->send( 400, "text/plain", "Bad parameter value" );
			return;
		}
	
		switch( sign<float>(x) ) {

			case 0:
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"",%s})json", transaction_details.data() );
				break;

			case 1:
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":%d,"ErrorMessage":"Only providing live data, please set to 0.",%s})json", 1023 + static_cast<byte>( ascom_error::InvalidValue ), transaction_details.data() );
				break;

			case -1:
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":%d,"ErrorMessage":"Value must be positive or 0",%s})json", 1023 + static_cast<byte>( ascom_error::InvalidValue ), transaction_details.data() );
				break;
		}

	} else

		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Sensor is not connected",%s})json", transaction_details.data() );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_observingconditions::build_timesincelastupdate_answer( char *sensor_name, const char *orig_sensor_name, etl::string<128> &transaction_details )
{
	time_t now;
	time( &now );

	switch( str2int( sensor_name )) {

		case str2int("pressure"):
		case str2int("temperature"):
		case str2int("humidity"):
		case str2int("dewpoint"):
			if ( !station.is_sensor_initialised( aws_device_t::BME_SENSOR ))
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1024,"ErrorMessage":"%s is not available",%s})json", orig_sensor_name, transaction_details.data() );
			else
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.1f,%s})json", (float)( now - station.get_sensor_data()->timestamp ), transaction_details.data() );
			break;

		case str2int("skybrightness"):
		case str2int("skyquality"):
			if ( !station.is_sensor_initialised( aws_device_t::TSL_SENSOR ))
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1024,"ErrorMessage":"%s is not available",%s})json", orig_sensor_name, transaction_details.data() );
			else
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.1f,%s})json", (float)( now - station.get_sensor_data()->timestamp ), transaction_details.data() );
			break;

		case str2int("cloudcover"):
		case str2int("skytemperature"):
			if ( !station.is_sensor_initialised( aws_device_t::MLX_SENSOR ))
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1024,"ErrorMessage":"%s is not available",%s})json", orig_sensor_name, transaction_details.data() );
			else
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.1f,%s})json", (float)( now - station.get_sensor_data()->timestamp ), transaction_details.data() );
			break;

		case str2int("rainrate"):
			if ( !station.is_sensor_initialised( aws_device_t::RAIN_SENSOR ))
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1024,"ErrorMessage":"%s is not available",%s})json", orig_sensor_name, transaction_details.data() );
			else
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.1f,%s})json", (float)( now - station.get_sensor_data()->timestamp ), transaction_details.data() );
			break;

		case str2int("windspeed"):
		case str2int("windgust"):
			if ( !station.is_sensor_initialised( aws_device_t::ANEMOMETER_SENSOR ))
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1024,"ErrorMessage":"%s is not available",%s})json", orig_sensor_name, transaction_details.data() );
			else
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.1f,%s})json", (float)( now - station.get_sensor_data()->timestamp ), transaction_details.data() );
			break;

		case str2int("winddirection"):
			if ( !station.is_sensor_initialised( aws_device_t::WIND_VANE_SENSOR ))
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1024,"ErrorMessage":"%s is not available",%s})json", orig_sensor_name, transaction_details.data() );
			else
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.1f,%s})json", (float)( now - station.get_sensor_data()->timestamp ), transaction_details.data() );
			break;

		case str2int(""):
			if ( station.is_sensor_initialised( aws_device_t::WIND_VANE_SENSOR ) ||
					station.is_sensor_initialised( aws_device_t::ANEMOMETER_SENSOR ) ||
					station.is_sensor_initialised( aws_device_t::RAIN_SENSOR ) ||
					station.is_sensor_initialised( aws_device_t::MLX_SENSOR ) ||
					station.is_sensor_initialised( aws_device_t::TSL_SENSOR ) ||
					station.is_sensor_initialised( aws_device_t::BME_SENSOR ))
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.1f,%s})json", (float)( now - station.get_sensor_data()->timestamp ), transaction_details.data() );
			else
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1024,"ErrorMessage":"No sensor is available",%s})json", transaction_details.data() );
			break;

		case str2int("starfwhm"):
			snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":1024,"ErrorMessage":"No such sensor: %s"})json", transaction_details.data(), orig_sensor_name );
			break;

		default:
			snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1025,"ErrorMessage":"","Value":"No such sensor name: %s",%s})json", orig_sensor_name, transaction_details.data() );
	}
}

void alpaca_observingconditions::build_property_description_answer( char *sensor_name, const char *orig_sensor_name, etl::string<128> &transaction_details )
{
	switch( str2int( sensor_name )) {

		case str2int("pressure"):
		case str2int("temperature"):
		case str2int("humidity"):
		case str2int("dewpoint"):
			if ( station.is_sensor_initialised( aws_device_t::BME_SENSOR ))
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":"BME280",%s})json", transaction_details.data() );
			else
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1024,"ErrorMessage":"%s sensor is not available",%s})json", orig_sensor_name, transaction_details.data() );
			break;

		case str2int("skybrightness"):
		case str2int("skyquality"):
			if ( station.is_sensor_initialised( aws_device_t::TSL_SENSOR ))
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":"TSL 2591",%s})json", transaction_details.data() );
			else
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1024,"ErrorMessage":"%s sensor is not available",%s})json", orig_sensor_name, transaction_details.data() );
			break;

		case str2int("cloudcover"):
		case str2int("skytemperature"):
			if ( station.is_sensor_initialised( aws_device_t::MLX_SENSOR ))
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":"MLX 96014",%s})json", transaction_details.data() );
			else
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1024,"ErrorMessage":"%s sensor is not available",%s})json", orig_sensor_name, transaction_details.data() );
			break;

		case str2int("rainrate"):
			if ( station.is_sensor_initialised( aws_device_t::RAIN_SENSOR ))
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":"Hydreon RG-9",%s})json", transaction_details.data() );
			else
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1024,"ErrorMessage":"%s sensor is not available",%s})json", orig_sensor_name, transaction_details.data() );
			break;

		case str2int("windspeed"):
		case str2int("windgust"):
			if ( station.is_sensor_initialised( aws_device_t::ANEMOMETER_SENSOR ))
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":"%s",%s})json", station.get_anemometer_sensorname(), transaction_details.data() );
			else
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1024,"ErrorMessage":"%s sensor is not available",%s})json", orig_sensor_name, transaction_details.data() );
			break;

		case str2int("winddirection"):
			if ( station.is_sensor_initialised( aws_device_t::WIND_VANE_SENSOR ))
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":"%s",%s})json", station.get_wind_vane_sensorname(), transaction_details.data() );
			else
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1024,"ErrorMessage":"%s sensor is not available",%s})json", orig_sensor_name, transaction_details.data() );
			break;

		case str2int("starfwhm"):
			snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":1024,"ErrorMessage":"No such sensor: %s"})json", transaction_details.data(), orig_sensor_name );
			break;

		default:
			snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1025,"ErrorMessage":"","Value":"No such sensor name",%s})json", transaction_details.data() );
	}
}

void alpaca_observingconditions::cloudcover( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( get_is_connected() && station.is_sensor_initialised( aws_device_t::MLX_SENSOR ))
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.1f,%s})json", station.get_sensor_data()->weather.cloud_cover, transaction_details.data() );
	else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1024,"ErrorMessage":"Sensor is not available",%s})json", transaction_details.data() );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_observingconditions::dewpoint( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( get_is_connected() && station.is_sensor_initialised( aws_device_t::BME_SENSOR ))
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%2.1f,%s})json", station.get_sensor_data()->weather.dew_point, transaction_details.data() );
	else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1024,"ErrorMessage":"Sensor is not available",%s})json", transaction_details.data() );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_observingconditions::humidity( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( get_is_connected() && station.is_sensor_initialised( aws_device_t::BME_SENSOR ))
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.1f,%s})json", station.get_sensor_data()->weather.rh, transaction_details.data() );
	else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1024,"ErrorMessage":"Sensor is not available",%s})json", transaction_details.data() );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_observingconditions::pressure( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( get_is_connected() && station.is_sensor_initialised( aws_device_t::BME_SENSOR ))
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%4.1f,%s})json", station.get_sensor_data()->weather.pressure, transaction_details.data() );
	else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1024,"ErrorMessage":"Sensor is not available",%s})json", transaction_details.data() );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_observingconditions::rainrate( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( !station.has_device( aws_device_t::RAIN_SENSOR ))

		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":%d,"ErrorMessage":"The station has no rain sensor",%s})json", 1023 + static_cast<byte>( ascom_error::PropertyOrMethodNotImplemented ), transaction_details.data() );

	else {

		if ( get_is_connected() && station.is_sensor_initialised( aws_device_t::RAIN_SENSOR )) {

			short x = station.get_sensor_data()->weather.rain_intensity;

			if ( x >= 0 )
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.1f,%s})json", rain_rate[ x ], transaction_details.data() );
			else
				snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":%d,"ErrorMessage":"Rain sensor data is temporarily unavailable",%s})json", 1023 + static_cast<byte>( ascom_error::PropertyOrMethodNotImplemented ), transaction_details.data() );

		} else

			snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Sensor is not connected",%s})json", transaction_details.data() );
	}

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_observingconditions::refresh( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( get_is_connected() ) {

		if ( station.poll_sensors() )

			snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"",%s})json", transaction_details.data() );

		else {

			request->send( 400, "application/json", "Could not refresh sensors" );
			return;

		}

	} else

		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Sensor is not connected",%s})json", transaction_details.data() );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_observingconditions::sensordescription( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( get_is_connected() ) {

		bool ok = false;
		for( int i = 0; i < request->params(); i++ ) {

			if ( !strcasecmp( request->getParam(i)->name().c_str(), "SensorName" )) {

				etl::string<32> sensor_name( request->getParam(i)->value().c_str() );
				etl::to_lower_case( sensor_name );
				ok = true;
				build_property_description_answer( sensor_name.data(), request->getParam(i)->name().c_str(), transaction_details );
			}
		}
		if ( !ok ) {

			request->send( 400, "application/json", "Missing SensorName parameter key" );
			return;
		}

	} else

		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"ObservingConditions device is not connected",%s})json", transaction_details.data() );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_observingconditions::set_connected( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( request->hasParam( "Connected", true ) ) {

		if ( !strcasecmp( request->getParam( "Connected", true )->value().c_str(), "true" )) {

			set_is_connected( true );
			snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details.data() );

		} else {

			if ( !strcasecmp( request->getParam( "Connected", true )->value().c_str(), "false" )) {

				set_is_connected( false );
				snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":0,"ErrorMessage":""})json", transaction_details.data() );

			} else

				snprintf( message_str.data(), message_str.capacity(), R"json({%s,"ErrorNumber":1025,"ErrorMessage":"Invalid value %s"})json", transaction_details.data(), request->getParam( "Connected", true )->value().c_str() );
		}
		request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
		return;
	}

	request->send( 400, "text/plain", "Missing Connected parameter" );
}

void alpaca_observingconditions::skybrightness( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( get_is_connected() && station.is_sensor_initialised( aws_device_t::TSL_SENSOR ))
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%6.4f,%s})json", (float)station.get_sensor_data()->sun.lux, transaction_details.data() );
	else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1024,"ErrorMessage":"Sensor is not available",%s})json", transaction_details.data() );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_observingconditions::skyquality( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( get_is_connected() && station.is_sensor_initialised( aws_device_t::TSL_SENSOR ))
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%2.2f,%s})json", station.get_sensor_data()->sqm.msas, transaction_details.data() );
	else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1024,"ErrorMessage":"Sensor is not available",%s})json", transaction_details.data() );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_observingconditions::skytemperature( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( get_is_connected() && station.is_sensor_initialised( aws_device_t::MLX_SENSOR ))
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%2.2f,%s})json", station.get_sensor_data()->weather.sky_temperature, transaction_details.data() );
	else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1024,"ErrorMessage":"Sensor is not available",%s})json", transaction_details.data() );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_observingconditions::temperature( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( get_is_connected() && station.is_sensor_initialised( aws_device_t::BME_SENSOR ))
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%2.2f,%s})json", station.get_sensor_data()->weather.temperature, transaction_details.data() );
	else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1024,"ErrorMessage":"Sensor is not available",%s})json", transaction_details.data() );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_observingconditions::timesincelastupdate( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( get_is_connected() ) {

		bool ok = false;
		for( int i = 0; i < request->params(); i++ ) {

			if ( !strcasecmp( request->getParam(i)->name().c_str(), "SensorName" )) {

				etl::string<32> sensor_name( request->getParam(i)->value().c_str() );
				etl::to_lower_case( sensor_name );
				ok = true;
				build_timesincelastupdate_answer( sensor_name.data(), request->getParam(i)->value().c_str(), transaction_details );
			}
		}
		if ( !ok ) {

			request->send( 400, "application/json", "Missing SensorName parameter key" );
			return;
		}

	} else

		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1031,"ErrorMessage":"Device is not connected",%s})json", transaction_details.data() );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_observingconditions::winddirection( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	uint16_t x = station.get_sensor_data()->weather.wind_direction;

	if ( station.get_sensor_data()->weather.wind_speed > 0 )
		x = ( x == 0 ) ? 360 : x;
	else
		x = 0;

	if ( get_is_connected() && station.is_sensor_initialised( aws_device_t::WIND_VANE_SENSOR ))
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.1f,%s})json", (float)x, transaction_details.data() );
	else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1024,"ErrorMessage":"No sensor is available",%s})json", transaction_details.data() );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_observingconditions::windgust( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( get_is_connected() && station.is_sensor_initialised( aws_device_t::ANEMOMETER_SENSOR ))
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.1f,%s})json", station.get_sensor_data()->weather.wind_gust, transaction_details.data() );
	else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1024,"ErrorMessage":"No sensor is available",%s})json", transaction_details.data() );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}

void alpaca_observingconditions::windspeed( AsyncWebServerRequest *request, etl::string<128> &transaction_details )
{
	if ( get_is_connected() && station.is_sensor_initialised( aws_device_t::ANEMOMETER_SENSOR ))
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":0,"ErrorMessage":"","Value":%3.1f,%s})json", station.get_sensor_data()->weather.wind_speed, transaction_details.data() );
	else
		snprintf( message_str.data(), message_str.capacity(), R"json({"ErrorNumber":1024,"ErrorMessage":"No sensor is available",%s})json", transaction_details.data() );

	request->send( 200, "application/json", static_cast<const char *>( message_str.data() ) );
}
