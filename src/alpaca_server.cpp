/*
	alpaca.cpp

	ASCOM ALPACA Server for the AstroWeatherStation (c) 2023-2024 F.Lesage

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

#define _ASYNC_WEBSERVER_LOGLEVEL_		0	// NOSONAR
#define _ETHERNET_WEBSERVER_LOGLEVEL_	0	// NOSONAR

#ifndef ASYNCWEBSERVER_REGEX
#error "ASYNCWEBSERVER_REGEX not defined, please read build instructions!"
#endif

#include <Arduino.h>
#include <AsyncTCP.h>
#include <Ethernet.h>
#include <SSLClient.h>
#include <AsyncUDP_ESP32_W5500.hpp>
#include <ESPAsyncWebSrv.h>
#include <SPIFFS.h>

#include "common.h"
#include "defaults.h"
#include "device.h"
#include "dome.h"
#include "alpaca_dome.h"
#include "alpaca_observingconditions.h"
#include "alpaca_safetymonitor.h"
#include "alpaca_telescope.h"
#include "alpaca_server.h"
#include "AstroWeatherStation.h"

// Keep this, as otherwise it will be the value defined in http_parser.h that will be taken ( = 4 ) and not the one expected by ESPAsyncWebSrv
#define	HTTP_PUT	( 8 )		// NOSONAR

extern AstroWeatherStation station;

const configured_device_t configured_devices[ CONFIGURED_DEVICES ] = {

	{ "Rain sensor", "Safetymonitor", 0, SAFETYMONITOR_UUID },
	{ "Dome", "Dome", 0, DOME_UUID },
	{ "AstroWeatherstation", "ObservingConditions", 0, AWS_UUID },
	{ "Fake telescope", "Telescope", 0, TELESCOPE_UUID }
};

constexpr unsigned int str2int(const char* str, int h = 0 )
{
    return !str[h] ? 5381 : (str2int(str, h+1) * 33) ^ str[h];
}

// flawfinder: ignore
const char *ascom_error_message[8] = {
	"Property or method not implemented",
	"Invalid value",
	"Value not set",
	"Not connected",
	"Invalid while parked",
	"Invalid while slaved",
	"Invalid operation",
	"Action not implemented"
};

alpaca_server::alpaca_server( void )
{
	memset( transaction_details, 0, 128 );
}

void alpaca_server::alpaca_getapiversions( AsyncWebServerRequest *request )
{
	etl::string<128> str;

	server_transaction_id++;

	if ( extract_transaction_details( request, false ) ) {

		snprintf( static_cast<char *>( str.data() ), str.capacity(), "{\"Value\":[1],%s}", transaction_details );
		request->send( 200, "application/json", static_cast<const char *>( str.data() ) );

	} else {

		if ( bad_request )
			request->send( 400, "text/plain", transaction_details );
		else
			request->send( 200, "application/json", static_cast<const char *>( str.data() ) );

	}
}

void alpaca_server::alpaca_getconfigureddevices( AsyncWebServerRequest *request )
{
	etl::string<1024> str;

	server_transaction_id++;

	if ( extract_transaction_details( request, false ) ) {

		if ( get_configured_devices( str.data(), 960 )) {

			etl::string<2048> str2;
			snprintf( str2.data(), str2.capacity(), "{\"Value\":%s,%s}", str.data(), transaction_details );
			request->send( 200, "application/json", static_cast<const char *>( str2.data() ) );

		} else

			request->send( 500, "text/plain", "Error while building answer." );

	} else {

		if ( bad_request )
			request->send( 400, "text/plain", transaction_details );
		else
			request->send( 200, "application/json", static_cast<const char *>( str.data() ) );
	}
}

void alpaca_server::alpaca_getdescription( AsyncWebServerRequest *request )
{
	server_transaction_id++;
	etl::string<256> str;

	snprintf( static_cast<char *>( str.data() ), str.capacity(), R"json({"Value":{"ServerName":"AWS","Manufacturer":"L-OpenAstroDevices","ManufacturerVersion":"%s","Location":"%s"},%s})json", station.get_unique_build_id().data() , station.get_location().data(), R"json("ClientTransactionID":0,"ServerTransactionID":0)json");
Serial.printf("DESC=[%s]\n",str.data());
	request->send( 200, "application/json", static_cast<const char *>( str.data() ) );
}

void alpaca_server::alpaca_getsetup( AsyncWebServerRequest *request )
{
	request->send( SPIFFS, "/ascom_home.html" );
}

void alpaca_server::dispatch_dome_request( AsyncWebServerRequest *request )
{
	switch( str2int( request->pathArg(1).c_str() )) {

		case str2int( "abortslew" ):
			if ( request->method() != HTTP_GET )
				dome.abortslew( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "canfindhome" ):
		case str2int( "canpark" ):
		case str2int( "cansetaltitude" ):
		case str2int( "cansetazimuth" ):
		case str2int( "cansetpark" ):
		case str2int( "canslave" ):
		case str2int( "cansyncazimuth" ):
		case str2int( "slewing" ):

			if ( request->method() == HTTP_GET )
				dome.send_value( request, transaction_details, false );
			else
				does_not_exist( request );
			break;

		case str2int( "cansetshutter" ):
			if ( request->method() == HTTP_GET )
				dome.cansetshutter( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "closeshutter" ):
			if ( request->method() == HTTP_GET )
				not_implemented( request, "No closing control over the roll-off top" );
			else
				dome.closeshutter( request, transaction_details );
			break;

		case str2int( "connected" ):
			if ( request->method() == HTTP_GET )
				dome.send_connected( request, transaction_details );
			else
				dome.set_connected( request, transaction_details );
			break;

		case str2int( "description" ):
			if ( request->method() == HTTP_GET )
				dome.send_description( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "driverinfo" ):
			if ( request->method() == HTTP_GET )
				dome.send_driverinfo( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "driverversion" ):
			if ( request->method() == HTTP_GET )
				dome.send_driverversion( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "interfaceversion" ):
			if ( request->method() == HTTP_GET )
				dome.send_interfaceversion( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "name" ):
			if ( request->method() == HTTP_GET )
				dome.send_name( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "openshutter" ):
			if ( request->method() == HTTP_GET )
				not_implemented( request, NULL );
			else
				dome.openshutter( request, transaction_details );
			break;

		case str2int( "shutterstatus" ):
			if ( request->method() == HTTP_GET )
				dome.shutterstatus( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "slaved" ):
			if ( request->method() == HTTP_GET )
				dome.send_value( request, transaction_details, false );
			else
				not_implemented( request, "Cannot slave roll-off roof to telescope" );
			break;

		case str2int( "supportedactions" ):
			if ( request->method() == HTTP_GET )
				dome.send_supportedactions( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "altitude" ):
		case str2int( "athome" ):
		case str2int( "atpark" ):
		case str2int( "azimuth" ):
		case str2int( "findhome" ):
		case str2int( "park" ):
		case str2int( "setpark" ):
		case str2int( "slewtoaltitude" ):
		case str2int( "slewtoazimuth" ):
		case str2int( "synctoazimuth" ):
			not_implemented( request, NULL );
			break;

		default:
			does_not_exist( request );
	}
}

void alpaca_server::dispatch_request( AsyncWebServerRequest *request )
{
	if ( !extract_transaction_details( request, ( request->method() != HTTP_GET ) )) {

		request->send( 400, "text/plain", transaction_details );
		return;
	}

	switch( str2int( request->pathArg(0).c_str() )) {

		case str2int( "dome" ):
			dispatch_dome_request( request );
			break;

		case str2int( "observingconditions" ):
			dispatch_observingconditions_request( request );
			break;

		case str2int( "safetymonitor" ):
			dispatch_safetymonitor_request( request );
			break;

		case str2int( "telescope", 0 ):
			dispatch_telescope_request( request );
			break;
	}
}

void alpaca_server::dispatch_safetymonitor_request( AsyncWebServerRequest *request )
{
	switch( str2int( request->pathArg(1).c_str() )) {

		case str2int( "connected" ):
			if ( request->method() == HTTP_GET )
				safety_monitor.send_connected( request, transaction_details );
			else
				safety_monitor.set_connected( request, transaction_details );
			break;

		case str2int( "description" ):
			if ( request->method() == HTTP_GET )
				safety_monitor.send_description( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "driverinfo" ):
			if ( request->method() == HTTP_GET )
				safety_monitor.send_driverinfo( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "driverversion" ):
			if ( request->method() == HTTP_GET )
				safety_monitor.send_driverversion( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "interfaceversion" ):
			if ( request->method() == HTTP_GET )
				safety_monitor.send_interfaceversion( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "issafe" ):
			if ( request->method() == HTTP_GET )
				safety_monitor.issafe( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "name" ):
			if ( request->method() == HTTP_GET )
				safety_monitor.send_name( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "supportedactions" ):
			if ( request->method() == HTTP_GET )
				safety_monitor.send_supportedactions( request, transaction_details );
			else
				does_not_exist( request );
			break;

		default:
			does_not_exist( request );
	}
}

void alpaca_server::dispatch_observingconditions_request( AsyncWebServerRequest *request )
{
	switch( str2int( request->pathArg(1).c_str() )) {

		case str2int( "averageperiod" ):
			if ( request->method() == HTTP_GET )
				observing_conditions.get_averageperiod( request, transaction_details );
			else
				observing_conditions.set_averageperiod( request, transaction_details );
			break;

		case str2int( "cloudcover" ):
			if ( request->method() == HTTP_GET )
				observing_conditions.cloudcover( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "dewpoint" ):
			if ( request->method() == HTTP_GET )
				observing_conditions.dewpoint( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "humidity" ):
			if ( request->method() == HTTP_GET )
				observing_conditions.humidity( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "pressure" ):
			if ( request->method() == HTTP_GET )
				observing_conditions.pressure( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "rainrate" ):
			if ( request->method() == HTTP_GET )
				observing_conditions.rainrate( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "skybrightness" ):
			if ( request->method() == HTTP_GET )
				observing_conditions.skybrightness( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "skyquality" ):
			if ( request->method() == HTTP_GET )
				observing_conditions.skyquality( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "skytemperature" ):
			if ( request->method() == HTTP_GET )
				observing_conditions.skytemperature( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "temperature" ):
			if ( request->method() == HTTP_GET )
				observing_conditions.temperature( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "winddirection" ):
			if ( request->method() == HTTP_GET )
				observing_conditions.winddirection( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "windgust" ):
			if ( request->method() == HTTP_GET )
				observing_conditions.windgust( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "windspeed" ):
			if ( request->method() == HTTP_GET )
				observing_conditions.windspeed( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "refresh" ):
			if ( request->method() != HTTP_GET )
				observing_conditions.refresh( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "sensordescription" ):
			if ( request->method() == HTTP_GET )
				observing_conditions.sensordescription( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "timesincelastupdate" ):
			if ( request->method() == HTTP_GET )
				observing_conditions.timesincelastupdate( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "connected" ):
			if ( request->method() == HTTP_GET )
				observing_conditions.send_connected( request, transaction_details );
			else
				observing_conditions.set_connected( request, transaction_details );
			break;

		case str2int( "description" ):
			if ( request->method() == HTTP_GET )
				observing_conditions.send_description( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "driverinfo" ):
			if ( request->method() == HTTP_GET )
				observing_conditions.send_driverinfo( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "driverversion" ):
			if ( request->method() == HTTP_GET )
				observing_conditions.send_driverversion( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "interfaceversion" ):
			if ( request->method() == HTTP_GET )
				observing_conditions.send_interfaceversion( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "name" ):
			if ( request->method() == HTTP_GET )
				observing_conditions.send_name( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "supportedactions" ):
			if ( request->method() == HTTP_GET )
				observing_conditions.send_supportedactions( request , transaction_details);
			else
				does_not_exist( request );
			break;

		case str2int( "starfwhm" ):
			not_implemented( request, "No sensor to measure star FWHM" );

		default:
			does_not_exist( request );
	}
}

void alpaca_server::dispatch_telescope_request( AsyncWebServerRequest *request )
{
	switch( str2int( request->pathArg(1).c_str() )) {

		case str2int( "athome" ):
			if ( request->method() == HTTP_GET )
				telescope.send_value( request, transaction_details, false );
			else
				does_not_exist( request );
			break;

		case str2int( "atpark" ):
			if ( request->method() == HTTP_GET )
				telescope.send_value( request, transaction_details, false );
			else
				does_not_exist( request );
			break;

		case str2int( "canfindhome" ):
			if ( request->method() == HTTP_GET )
				telescope.send_value( request, transaction_details, false );
			else
				does_not_exist( request );
			break;

		case str2int( "canpark" ):
			if ( request->method() == HTTP_GET )
				telescope.send_value( request, transaction_details, false );
			else
				does_not_exist( request );
			break;

		case str2int( "canpulseguide" ):
			if ( request->method() == HTTP_GET )
				telescope.send_value( request, transaction_details, false );
			else
				does_not_exist( request );
			break;

		case str2int( "cansetdeclinationrate" ):
			if ( request->method() == HTTP_GET )
				telescope.send_value( request, transaction_details, false );
			else
				does_not_exist( request );

		case str2int( "cansetguiderates" ):
			if ( request->method() == HTTP_GET )
				telescope.send_value( request, transaction_details, false );
			else
				does_not_exist( request );
			break;

		case str2int( "cansetpark" ):
			if ( request->method() == HTTP_GET )
				telescope.send_value( request, transaction_details, false );
			else
				does_not_exist( request );
			break;

		case str2int( "cansetpierside" ):
			if ( request->method() == HTTP_GET )
				telescope.send_value( request, transaction_details, false );
			else
				does_not_exist( request );
			break;

		case str2int( "cansetrightascensionrate" ):
			if ( request->method() == HTTP_GET )
				telescope.send_value( request, transaction_details, false );
			else
				does_not_exist( request );
			break;

		case str2int( "cansettracking" ):
			if ( request->method() == HTTP_GET )
				telescope.send_value( request, transaction_details, false );
			else
				does_not_exist( request );
			break;

		case str2int( "canslew" ):
			if ( request->method() == HTTP_GET )
				telescope.send_value( request, transaction_details, false );
			else
				does_not_exist( request );
			break;

		case str2int( "canslewaltaz" ):
			if ( request->method() == HTTP_GET )
				telescope.send_value( request, transaction_details, false );
			else
				does_not_exist( request );
			break;

		case str2int( "canslewaltazasync" ):
			if ( request->method() == HTTP_GET )
				telescope.send_value( request, transaction_details, false );
			else
				does_not_exist( request );
			break;

		case str2int( "canslewasync" ):
			if ( request->method() == HTTP_GET )
				telescope.send_value( request, transaction_details, false );
			else
				does_not_exist( request );
			break;

		case str2int( "cansync" ):
			if ( request->method() == HTTP_GET )
				telescope.send_value( request, transaction_details, false );
			else
				does_not_exist( request );
			break;

		case str2int( "cansyncaltaz" ):
			if ( request->method() == HTTP_GET )
				telescope.send_value( request, transaction_details, false );
			else
				does_not_exist( request );
			break;

		case str2int( "canunpark" ):
			if ( request->method() == HTTP_GET )
				telescope.send_value( request, transaction_details, false );
			else
				does_not_exist( request );
			break;

		case str2int( "declination" ):
			if ( request->method() == HTTP_GET )
				telescope.send_value( request, transaction_details, 0.0 );
			else
				does_not_exist( request );
			break;

		case str2int( "equatorialsystem" ):
			if ( request->method() == HTTP_GET )
				telescope.send_value( request, transaction_details, (byte)0 );
			else
				does_not_exist( request );
			break;

		case str2int( "declinationrate" ):
			if ( request->method() == HTTP_GET )
				telescope.send_value( request, transaction_details, 0.0 );
			else
				not_implemented( request, "This is a fake telescope" );
			break;

		case str2int( "rightascension" ):
			if ( request->method() == HTTP_GET )
				telescope.send_value( request, transaction_details, 0.0 );
			else
				does_not_exist( request );
			break;

		case str2int( "rightascensionrate" ):
			if ( request->method() == HTTP_GET )
				telescope.send_value( request, transaction_details, 0.0 );
			else
				not_implemented( request, "This is a fake telescope" );
			break;

		case str2int( "siderealtime" ):
			if ( request->method() == HTTP_GET )
				telescope.siderealtime( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "siteelevation" ):
			if ( request->method() == HTTP_GET )
				telescope.siteelevation( request, transaction_details );
			else
				telescope.set_siteelevation( request, transaction_details );
			break;

		case str2int( "sitelatitude" ):
			if ( request->method() == HTTP_GET )
				telescope.sitelatitude( request, transaction_details );
			else
				telescope.set_sitelatitude( request, transaction_details );
			break;

		case str2int( "sitelongitude" ):
			if ( request->method() == HTTP_GET )
				telescope.sitelongitude( request, transaction_details );
			else
				telescope.set_sitelongitude( request, transaction_details );
			break;

		case str2int( "tracking" ):
			if ( request->method() == HTTP_GET )
				telescope.send_value( request , transaction_details, false );
			else
				not_implemented( request, "This is a fake telescope" );
			break;

		case str2int( "trackingrate" ):
			if ( request->method() == HTTP_GET )
				telescope.send_value( request , transaction_details, (byte)0 );
			else
				not_implemented( request, "This is a fake telescope" );
			break;

		case str2int( "trackingrates" ):
			if ( request->method() == HTTP_GET )
				telescope.trackingrates( request , transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "utcdate" ):
			if ( request->method() == HTTP_GET )
				telescope.utcdate( request , transaction_details );
			else
				telescope.set_utcdate( request , transaction_details);
			break;

		case str2int( "abortslew" ):
			if ( request->method() != HTTP_GET )
				telescope.send_value( request , transaction_details, true );
			else
				does_not_exist( request );
			break;

		case str2int( "axisrates" ):
			if ( request->method() == HTTP_GET )
				telescope.axisrates( request , transaction_details);
			else
				does_not_exist( request );
			break;

		case str2int( "canmoveaxis" ):
			if ( request->method() == HTTP_GET )
				telescope.send_value( request , transaction_details, false );
			else
				does_not_exist( request );
			break;

		case str2int( "connected" ):
			if ( request->method() == HTTP_GET )
				telescope.send_connected( request, transaction_details );
			else
				telescope.set_connected( request, transaction_details );
			break;

		case str2int( "description" ):
			if ( request->method() == HTTP_GET )
				telescope.send_description( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "driverinfo" ):
			if ( request->method() == HTTP_GET )
				telescope.send_driverinfo( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "driverversion" ):
			if ( request->method() == HTTP_GET )
				telescope.send_driverversion( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "interfaceversion" ):
			if ( request->method() == HTTP_GET )
				telescope.send_interfaceversion( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "name" ):
			if ( request->method() == HTTP_GET )
				telescope.send_name( request, transaction_details );
			else
				does_not_exist( request );
			break;

		case str2int( "supportedactions" ):
			if ( request->method() == HTTP_GET )
				telescope.send_supportedactions( request , transaction_details);
			else
				does_not_exist( request );
			break;

		case str2int( "alignmentmode" ):
		case str2int( "altitude" ):
		case str2int( "aperturearea" ):
		case str2int( "aperturediameter" ):
		case str2int( "azimuth" ):
		case str2int( "doesrefraction" ):
		case str2int( "focallength" ):
		case str2int( "guideratedeclination" ):
		case str2int( "guideraterightascension" ):
		case str2int( "ispulseguiding" ):
		case str2int( "sideofpier" ):
		case str2int( "slewing" ):
		case str2int( "slewsettletime" ):
		case str2int( "targetdeclination" ):
		case str2int( "targetrightascension" ):
		case str2int( "destinationsideofpier" ):
		case str2int( "findhome" ):
		case str2int( "moveaxis" ):
		case str2int( "park" ):
		case str2int( "pulseguide" ):
		case str2int( "setpark" ):
		case str2int( "slewtoaltaz" ):
		case str2int( "slewtoaltazsync" ):
		case str2int( "slewtocoordinates" ):
		case str2int( "slewtocoordinatesasync" ):
		case str2int( "slewtotarget" ):
		case str2int( "slewtotargetasync" ):
		case str2int( "synctotarget" ):
		case str2int( "unpark" ):
			not_implemented( request, NULL );

		default:
			does_not_exist( request );
	}
}

void alpaca_server::does_not_exist( AsyncWebServerRequest *request )
{
	int params = request->params();

	server_transaction_id++;
	if ( debug_mode ) {

		Serial.printf( "\n[DEBUG] ALPACA: unimplemented endpoint: %x %s, with parameters: ", request->method(), request->url().c_str());
		for( int i = 0; i < params; i++ )
			  Serial.printf( "(%s=%s) ", request->getParam(i)->name().c_str(), request->getParam(i)->value().c_str() );
		  Serial.printf("\n");
  	}
	request->send( 400, "application/json", "Endpoint does not exist" );
}

void alpaca_server::get_config( AsyncWebServerRequest *request )
{
	etl::string_view json_string = station.get_json_string_config();

	server_transaction_id++;
	if ( json_string.size() ) {

		request->send( 200, "application/json", json_string.data() );

	} else

		request->send( 500, "text/plain", "[ERROR] get_configuration() had a problem, please contact support." );
}

bool alpaca_server::get_configured_devices( char *json_string, size_t len )
{
	etl::string<256> buff;
	size_t	l = 1;
	size_t	l2;

	*json_string = '[';
	*(json_string+1) = 0;

	for( uint8_t i = 0; i < CONFIGURED_DEVICES; i++ ) {

		l += snprintf( buff.data(), buff.capacity(), R"json({"DeviceName":"%s","DeviceType":"%s","DeviceNumber":%d,"UniqueID":"%s"},)json", configured_devices[i].DeviceName, configured_devices[i].DeviceType, configured_devices[i].DeviceNumber, configured_devices[i].UniqueID );
		if ( l <= len )
			l2 = strlcat( json_string, buff.data(), len );
		else {
			return false;
		}

	}
	json_string[ l2 - 1 ] = 0;
	if ( l <= len - 2 )
		strlcat( json_string, "]", len );
	else
		return false;
		Serial.printf("CONFIGDEV=%s\n",json_string);
	return true;
}

bool alpaca_server::extract_transaction_details( AsyncWebServerRequest *request, bool post )
{
	transaction_status = ascom_error::Ok;
	bad_request = false;

	for( int i = 0; i < request->params(); i++ ) {

		if ( !strcasecmp( request->getParam(i)->name().c_str(), "ClientID" )) {

				// flawfinder: ignore
				if ( ( client_id = atoi( request->getParam(i)->value().c_str() )) <= 0 ) {

					bad_request = true;
					snprintf( transaction_details, 127, "Missing or invalid ClientID" );
					client_id = 0;
					break;
				}
		}

		if ( !strcasecmp( request->getParam(i)->name().c_str(), "ClientTransactionID" )) {

				// flawfinder: ignore
				if ( ( client_transaction_id = atoi( request->getParam(i)->value().c_str() )) <= 0 ) {

					bad_request = true;
					snprintf( transaction_details, 127, "Missing or invalid ClientTransactionID" );
					client_transaction_id = 0;
					break;
				}
		}
	}

	if ( debug_mode ) {

		Serial.printf( "\n[DEBUG] Alpaca client request parameters: [Method:%02d] ", request->method() );
		for( int i = 0; i < request->params(); i++ )
			Serial.printf( "(%s=%s)", request->getParam(i)->name().c_str(),request->getParam(i)->value().c_str() );
		Serial.printf( "\n" );
	}

	if ( bad_request )
		return false;

	server_transaction_id++;
	snprintf( transaction_details, 127, R"json("ClientID":%d,"ClientTransactionID":%d,"ServerTransactionID":%d)json", client_id, client_transaction_id, server_transaction_id );
	return true;
}

void alpaca_server::not_implemented( AsyncWebServerRequest *request, const char *msg )
{
	etl::string<256> str;
	server_transaction_id++;
	
	if ( debug_mode )
		Serial.printf( "\n[DEBUG] Alpaca : not implemented endpoint: %s\n", request->url().c_str());

	if ( extract_transaction_details( request, false ) ) {

		snprintf( static_cast<char *>( str.data() ), str.capacity(), R"json({%s,"ErrorNumber":1024,"ErrorMessage":"%s"})json", transaction_details, msg?msg:"" );
		request->send( 200, "application/json", static_cast<const char *>( str.data() ) );

	} else {

		if ( bad_request )
			request->send( 400, "text/plain", transaction_details );
		else
			request->send( 200, "application/json", static_cast<const char *>( str.data() ) );

	}
}

void alpaca_server::on_packet( AsyncUDPPacket packet )
{
Serial.printf("ONPACKET\n");
	if ( packet.length() ) {
Serial.printf("OK1\n");

		// flawfinder: ignore
		int len = packet.read( reinterpret_cast<uint8_t *>( buf ), 255 );
Serial.printf("READ %d bytes in packet [%s]\n",len,buf);
		if ( len > 0 )
			buf[ len ] = 0;
		if ( len < 16 ) {
			if ( debug_mode )
				Serial.printf( "[INFO] ALPACA Discovery: short packet [%s] from %s:%d\n" , buf, packet.remoteIP().toString().c_str(), packet.remotePort() );
			return;
		}
		if ( strncmp( "alpacadiscovery1", buf, 16 ) != 0 ) {
			if ( debug_mode )
				Serial.printf( "[INFO] ALPACA Discovery: bad header [%s] from %s:%d\n",  buf, packet.remoteIP().toString().c_str(), packet.remotePort() );
			return;
		}
		int l = snprintf( buf, 255, "{\"AlpacaPort\":%d}", ALPACA_SERVER_PORT );
		if ( l > 0 )
			ascom_discovery.writeTo( reinterpret_cast<uint8_t *>( buf ), l, packet.remoteIP(), packet.remotePort(), packet.interface() );
		else
			Serial.printf( "[ERROR] ALPACA: Could not build discovery answer. Please contact support.\n" );
	}
}

void alpaca_server::send_file( AsyncWebServerRequest *request )
{
	if ( !SPIFFS.exists( request->url().c_str() )) {

		etl::string<64> msg;
		Serial.printf( "[ERROR] File [%s] not found.", request->url().c_str() );
		snprintf( msg.data(), msg.capacity(), "[ERROR] File [%s] not found.", request->url().c_str() );
		request->send( 500, "text/html", msg.data() );
		return;
	}

	request->send( SPIFFS, request->url().c_str() );
	delay( 500 );
}
	
bool alpaca_server::start( IPAddress address, bool _debug_mode )
{
	debug_mode = _debug_mode;

	dome.set_debug_mode( debug_mode );
	telescope.set_debug_mode( debug_mode );
	observing_conditions.set_debug_mode( debug_mode );
	safety_monitor.set_debug_mode( debug_mode );
	
	if ( !server_up ) {

		server = new AsyncWebServer( ALPACA_SERVER_PORT );
		if ( server )
			Serial.printf("[INFO] Started ALPACA server.\n" );
		else
			Serial.printf("[ERROR] Failed to start ALPACA server.\n" );

	}
	if ( !ascom_discovery.listen( 32227 ))

		Serial.printf( "[ERROR] Could not start ALPACA discovery service.\n" );
		
	else {	

		if ( debug_mode )
			Serial.printf( "[INFO] ALPACA discovery server started on %s:%d\n", address.toString().c_str(), 32227 );

		ascom_discovery.onPacket( std::bind( &alpaca_server::on_packet, this, std::placeholders::_1 ));
	}
	
	server->on( "/get_config", HTTP_GET, std::bind( &alpaca_server::get_config, this, std::placeholders::_1 ));

	server->on( "/setup", HTTP_GET, std::bind( &alpaca_server::alpaca_getsetup, this, std::placeholders::_1 ));
	server->on( "/management/apiversions", HTTP_GET, std::bind( &alpaca_server::alpaca_getapiversions, this, std::placeholders::_1 ));
	server->on( "/management/v1/description", HTTP_GET, std::bind( &alpaca_server::alpaca_getdescription, this, std::placeholders::_1 ));
	server->on( "/management/v1/configureddevices", HTTP_GET, std::bind( &alpaca_server::alpaca_getconfigureddevices, this, std::placeholders::_1 ));
	server->on( "/favicon.ico", HTTP_GET, std::bind( &alpaca_server::send_file, this, std::placeholders::_1 ));

	server->on( "^\\/api\\/v1\\/(dome|safetymonitor|telescope|observingconditions)\\/0\\/([a-z]+)$", HTTP_GET, std::bind( &alpaca_server::dispatch_request, this, std::placeholders::_1 ));
	server->on( "^\\/api\\/v1\\/(dome|safetymonitor|telescope|observingconditions)\\/0\\/([a-z]+)$", HTTP_PUT, std::bind( &alpaca_server::dispatch_request, this, std::placeholders::_1 ));

	server->on( "^\\/api\\/v1\\/([a-zA-Z]+)\\/([0-9]+)\\/.+$", HTTP_GET, std::bind( &alpaca_server::does_not_exist, this, std::placeholders::_1 ));

	server->onNotFound( std::bind( &alpaca_server::does_not_exist, this, std::placeholders::_1 ));
	server->begin();

	return true;
}
