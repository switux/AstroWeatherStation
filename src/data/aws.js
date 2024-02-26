let dashboardRefresh;
const wind_direction = [ 'N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW' ];
const MLX_SENSOR = 0x01;
const TSL_SENSOR = 0x02;
const BME_SENSOR = 0x04;
const WV_SENSOR = 0x08;
const WS_SENSOR = 0x10;
const RAIN_SENSOR = 0x20;

const	GPS_DEVICE	= 0x40;
const	DOME_DEVICE = 0x80;

// 12=After OTA Update
const	RESET_REASON = [ 'Unknown', 'Power on', '', 'SW reset', 'OWDT reset', 'Deep sleep reset', 'SDIO reset', 'Timer group 0 WD reset', 'Timer group 1 WD reset', 'RTC WD reset core', 'Intrusion reset', 'Time group reset CPU', 'SW reset CPU', 'RTC WD reset CPU', 'Reset by PRO CPU', 'Brown out', 'RTC WD Reset' ];
const	DOME_SHUTTER_STATUS = [ 'Open', 'Closed', 'Opening', 'Closing', 'Error' ];

function checkbox_change( param )
{
	switch( param.id ) {

		case 'has_ws':
			if ( !param.checked )
				document.querySelector('#anemometer_model').value = '';
			break;

		case 'has_wv':
			if ( !param.checked )
				document.querySelector('#wind_vane_model').value = '';
			break;
	}
}

function config_section_active( section_name, yes_no )
{
	if ( yes_no ) {

		document.getElementById( section_name ).style.display = 'flex';
		document.getElementById( section_name ).style.backgroundColor = '#f6f6f6';
		document.getElementById( "banner_"+section_name ).style.backgroundColor = "#f6f6f6";
		document.getElementById( "banner_"+section_name ).style.marginBottom = '0px';
		if ( section_name != 'general' )
			document.getElementById( section_name ).style.borderRadius = '5px 5px 5px 5px';
		else
			document.getElementById( section_name ).style.borderRadius = '0px 5px 5px 5px';

	} else {

		document.getElementById( section_name ).style.display = 'none';
		document.getElementById( section_name ).style.backgroundColor = '#dbdbdb';
		document.getElementById( "banner_"+section_name ).style.backgroundColor = "#dbdbdb";
		document.getElementById( "banner_"+section_name ).style.marginBottom = '2px';

	}
}

function fill_lookout_value( parameter, from_list, sensor_available, values )
{
	document.getElementById( parameter+"_active" ).checked = ( values[ parameter+"_active" ] == "1" )? 'true' : 'false';
	if ( from_list )
		document.querySelector( "#"+parameter+"_max").value = values[parameter+"_max" ];
	else
		document.getElementById( parameter+"_max" ).value = values[ parameter+"_max" ];
	document.getElementById( parameter+"_delay" ).value = values[ parameter+"_delay" ];
	if ( sensor_available )
		document.getElementById( parameter+"_active" ).checked = ( values[ parameter+"_missing" ] == "1" )? 'true' : 'false';
}

function fill_lookout_values( values )
{
	document.getElementById("lookout_enabled").checked = ( values['lookout_enabled'] ==  '1' )? 'true' : 'false';

	fill_lookout_value( "unsafe_wind_speed_1", false, true, values );
	fill_lookout_value( "unsafe_wind_speed_2", false, true, values );
	fill_lookout_value( "unsafe_cloud_coverage_1", true, true, values );
	fill_lookout_value( "unsafe_cloud_coverage_2", true, true, values );
	fill_lookout_value( "unsafe_rain_intensity", true, true, values );

	fill_lookout_value( "safe_wind_speed", false, false, values );
	fill_lookout_value( "safe_cloud_coverage_1", true, false, values );
	fill_lookout_value( "safe_cloud_coverage_2", true, false, values );
	fill_lookout_value( "safe_rain_intensity", false, false, values );
}

function fill_network_values( values )
{
	switch( values['wifi_mode'] ) {
		case 'AP':
			document.getElementById("AP").checked = true;
			break;
		case 'Client':
			document.getElementById("Client").checked = true;
			break;
		case 'Both':
			document.getElementById("Both").checked = true;
			break;
	}
	document.getElementById("eth_ip").value = values['eth_ip'];
	document.getElementById("eth_gw").value = values['eth_gw'];
	document.getElementById("eth_dns").value = values['eth_dns'];
	document.getElementById("ap_ssid").value = values['ap_ssid'];
	document.getElementById("wifi_ap_password").value = values['wifi_ap_password'];
	document.getElementById("wifi_ap_ip").value = values['wifi_ap_ip'];
	document.getElementById("wifi_ap_gw").value = values['wifi_ap_gw'];
	document.getElementById("wifi_ap_dns").value = values['wifi_ap_dns'];
	document.getElementById("sta_ssid").value = values['sta_ssid'];
	document.getElementById("wifi_sta_password").value = values['wifi_sta_password'];
	document.getElementById("wifi_sta_ip").value = values['wifi_sta_ip'];
	document.getElementById("wifi_sta_gw").value = values['wifi_sta_gw'];
	document.getElementById("wifi_sta_dns").value = values['wifi_sta_dns'];
	document.getElementById("remote_server").value = values['remote_server'];
	document.getElementById("url_path").value = values['url_path'];

	if ( values['has_ethernet'] === true )
		document.getElementById("iface_option").style.display = "table-row";
	else
		document.getElementById("iface_option").style.display = "none";

	if ( values['pref_iface'] == "2" ) {

		hide_wifi();
		document.getElementById("show_alpaca_interface").style.display = "none";
		document.getElementById("ethernet").checked = true;
		document.getElementById("alpaca_eth").checked = true;

	} else {

		show_wifi();
		document.getElementById("show_alpaca_interface").style.display = "table-row";
		document.getElementById("wifi").checked = true;
		document.getElementById("alpaca_eth").checked = false;

	}

	toggle_sta_ipgw(  values['show_wifi_sta_ip_mode'] );
	switch( values['wifi_mode'] ) {
		case 0:
			document.getElementById("AP").checked = true;
			break;
		case 1:
			document.getElementById("Client").checked = true;
			break;
		case 2:
			document.getElementById("Both").checked = true;
			break;
	}
	if ( values["eth_ip_mode"] == 1 )
		document.getElementById("eth_fixed").checked = true;
	else
		document.getElementById("eth_dhcp").checked = true;

}

function fill_sensor_values( values )
{
	document.getElementById("has_bme").checked = values['has_bme'];
	document.getElementById("has_tsl").checked = values['has_tsl'];
	document.getElementById("has_mlx").checked = values['has_mlx'];
	document.getElementById("has_ws").checked = values['has_ws'];
	document.getElementById("has_wv").checked = values['has_wv'];
	document.getElementById("has_rain_sensor").checked = values['has_rain_sensor'];
	document.getElementById("has_gps").checked = values['has_gps'];
	document.querySelector("#anemometer_model").value=values['anemometer_model'];
	document.querySelector("#wind_vane_model").value=values['wind_vane_model'];
}

function fill_cloud_coverage_parameter_values( values )
{
	document.getElementById("k1").value = values['k1'];
	document.getElementById("k2").value = values['k2'];
	document.getElementById("k3").value = values['k3'];
	document.getElementById("k4").value = values['k4'];
	document.getElementById("k5").value = values['k5'];
	document.getElementById("k6").value = values['k6'];
	document.getElementById("k7").value = values['k7'];
}

function force_ota()
{
	let req = new XMLHttpRequest();
	req.onreadystatechange = function() {

		if ( this.readyState == 4 && this.status == 200 ) {

			console.log( req.responseText );
			let values = JSON.parse( req.responseText );
		}
	};
	req.open( "GET", "/ota_update", true );
	req.send();

}

function hide_wifi()
{
	document.getElementById("show_wifi_mode").style.display = "none";
	document.getElementById("show_sta_ssid").style.display = "none";
	document.getElementById("show_wifi_sta_password").style.display = "none";
	document.getElementById("show_wifi_sta_ip_mode").style.display = "none";
	document.getElementById("show_wifi_sta_ip").style.display = "none";
	document.getElementById("show_wifi_sta_gw").style.display = "none";
	document.getElementById("show_wifi_sta_dns").style.display = "none";
	document.getElementById("show_ap_ssid").style.display = "none";
	document.getElementById("show_wifi_ap_password").style.display = "none";
	document.getElementById("show_wifi_ap_ip_mode").style.display = "none";
	document.getElementById("show_wifi_ap_ip").style.display = "none";
	document.getElementById("show_wifi_ap_gw").style.display = "none";
	document.getElementById("show_wifi_ap_dns").style.display = "none";
	document.getElementById("show_eth_ip_mode").style.display = "table-row";
	document.getElementById("show_eth_ip").style.display = "table-row";
	document.getElementById("show_eth_gw").style.display = "table-row";
	document.getElementById("show_eth_dns").style.display = "table-row";
}

function retrieve_display_data()
{
	toggle_panel( 'general' );

	let req = new XMLHttpRequest();
	req.onreadystatechange = function() {

		if ( this.readyState == 4 && this.status == 200 ) {

			let values = JSON.parse( req.responseText );
			document.getElementById("tzname").value = values['tzname'];
			fill_network_values( values );
			fill_sensor_values( values );
			fill_cloud_coverage_parameter_values( values );
			fill_lookout_values( values );

		}
	};
	req.open( "GET", "/get_config", true );
	req.send();

	let req2 = new XMLHttpRequest();
	req2.onreadystatechange = function() {
		if ( this.readyState == 4 && this.status == 200 ) {
			document.getElementById("root_ca").value = req2.responseText;
		}
	};
	req2.open( "GET", "/get_root_ca", true );
	req2.send();
}

function toggle_panel( panel_id )
{
	config_section_active( "general", false );
	config_section_active( "network", false );
	config_section_active( "sensors", false );
	config_section_active( "others", false );
	config_section_active( "lookout", false );
	config_section_active( "alpaca", false );
	config_section_active( "dashboard", false );
	config_section_active( panel_id, true );
	if ( panel_id == 'dashboard' ) {

		fetch_station_data();
		dashboardRefresh = setInterval( fetch_station_data, 10000 );

	} else
		clearInterval( dashboardRefresh );
}

function toggle_wifi_mode( wifi_mode )
{
	switch( wifi_mode ) {
		case 0:
			document.getElementById("show_ap_ssid").style.display = "none";
			document.getElementById("show_wifi_ap_password").style.display = "none";
			document.getElementById("show_wifi_ap_ip_mode").style.display = "none";
			document.getElementById("show_wifi_ap_ip").style.display = "none";
			document.getElementById("show_wifi_ap_gw").style.display = "none";
			document.getElementById("show_wifi_ap_dns").style.display = "none";
			document.getElementById("show_sta_ssid").style.display = "table-row";
			document.getElementById("show_wifi_sta_password").style.display = "table-row";
			document.getElementById("show_wifi_sta_ip_mode").style.display = "table-row";
			document.getElementById("show_wifi_sta_ip").style.display = "table-row";
			document.getElementById("show_wifi_sta_gw").style.display = "table-row";
			document.getElementById("show_wifi_sta_dns").style.display = "table-row";

			break;
		case 1:
			document.getElementById("show_sta_ssid").style.display = "none";
			document.getElementById("show_wifi_sta_password").style.display = "none";
			document.getElementById("show_wifi_sta_ip_mode").style.display = "none";
			document.getElementById("show_wifi_sta_ip").style.display = "none";
			document.getElementById("show_wifi_sta_gw").style.display = "none";
			document.getElementById("show_wifi_sta_dns").style.display = "none";
			document.getElementById("show_wifi_ap_password").style.display = "table-row";
			document.getElementById("show_wifi_ap_ip").style.display = "table-row";
			document.getElementById("show_wifi_ap_gw").style.display = "table-row";
			document.getElementById("show_wifi_ap_dns").style.display = "table-row";
			break;
		case 2:
			show_wifi();
			break;
	}
}

function toggle_sta_ipgw( show )
{
	if ( show ) {
		document.getElementById("wifi_sta_fixed").checked = true;
		document.getElementById("wifi_sta_ip").removeAttribute("readonly");
		document.getElementById("wifi_sta_gw").removeAttribute("readonly");
		document.getElementById("wifi_sta_dns").removeAttribute("readonly");
		document.getElementById("wifi_sta_ip").style.display.background = "#d0d0d0";
		document.getElementById("wifi_sta_gw").style.display.background = "#d0d0d0";
		document.getElementById("wifi_sta_dns").style.display.background = "#d0d0d0";
	} else {
		document.getElementById("wifi_sta_dhcp").checked = true;
		document.getElementById("wifi_sta_ip").setAttribute("readonly","true");
		document.getElementById("wifi_sta_gw").setAttribute("readonly","true");
		document.getElementById("wifi_sta_dns").setAttribute("readonly","true");
		document.getElementById("wifi_sta_ip").style.display.background = "white";
		document.getElementById("wifi_sta_gw").style.display.background = "white";
		document.getElementById("wifi_sta_dns").style.display.background = "white";
	}
}

function toggle_eth_ipgw( show )
{
	if ( show ) {
		document.getElementById("eth_fixed").checked = true;
		document.getElementById("eth_ip").removeAttribute("readonly");
		document.getElementById("eth_gw").removeAttribute("readonly");
		document.getElementById("eth_dns").removeAttribute("readonly");
		document.getElementById("eth_ip").style.display.background = "#d0d0d0";
		document.getElementById("eth_gw").style.display.background = "#d0d0d0";
		document.getElementById("eth_dns").style.display.background = "#d0d0d0";
	} else {
		document.getElementById("eth_dhcp").checked = true;
		document.getElementById("eth_ip").setAttribute("readonly","true");
		document.getElementById("eth_gw").setAttribute("readonly","true");
		document.getElementById("eth_dns").setAttribute("readonly","true");
		document.getElementById("eth_ip").style.display.background = "white";
		document.getElementById("eth_gw").style.display.background = "white";
		document.getElementById("eth_dns").style.display.background = "white";
	}
}

function reboot()
{
	let req = new XMLHttpRequest();
	req.open( "GET", "/reboot", true );
	req.send();
}

function send_config()
{
	let req = new XMLHttpRequest();
	req.open( "POST", "/set_config", true );
	req.setRequestHeader( "Content-Type", "application/json;charset=UTF-8" );
	req.send( JSON.stringify(Object.fromEntries( ( new FormData(document.querySelector('#config') )).entries())) );
}

function show_wifi()
{
	document.getElementById("show_wifi_mode").style.display = "table-row";
	document.getElementById("show_sta_ssid").style.display = "table-row";
	document.getElementById("show_wifi_sta_password").style.display = "table-row";
	document.getElementById("show_wifi_sta_ip_mode").style.display = "table-row";
	document.getElementById("show_wifi_sta_ip").style.display = "table-row";
	document.getElementById("show_wifi_sta_gw").style.display = "table-row";
	document.getElementById("show_wifi_sta_dns").style.display = "table-row";
	document.getElementById("show_ap_ssid").style.display = "table-row";
	document.getElementById("show_wifi_ap_password").style.display = "table-row";
	document.getElementById("show_wifi_ap_ip_mode").style.display = "none";
	document.getElementById("show_wifi_ap_ip").style.display = "table-row";
	document.getElementById("show_wifi_ap_gw").style.display = "table-row";
	document.getElementById("show_wifi_ap_dns").style.display = "table-row";
	document.getElementById("show_eth_ip_mode").style.display = "none";
	document.getElementById("show_eth_ip").style.display = "none";
	document.getElementById("show_eth_gw").style.display = "none";
	document.getElementById("show_eth_dns").style.display = "none";
}

function fetch_station_data()
{
	let req = new XMLHttpRequest();
	req.onreadystatechange = function() {
		if ( this.readyState == 4 && this.status == 200 ) {
			let values = JSON.parse( req.responseText );
			update_dashboard( values );
		}
	};
	req.open( "GET", "/get_station_data", true );
	req.send();
}

function update_dashboard( values )
{
	document.getElementById("battery_level").textContent = values['battery_level']+'%';
	let uptime = values['uptime'];
	let days = Math.floor( uptime / ( 3600 * 24 ));
	let hours = Math.floor( ( uptime % ( 3600 * 24 )) / 3600 );
	let mins = Math.floor( ( uptime % 3600 ) / 60 );
	let secs = uptime % 60;

	document.getElementById("uptime").textContent = days+' days '+hours+' hours '+mins+' minutes '+secs+' seconds';
	document.getElementById("reset_reason").textContent = RESET_REASON[ values['reset_reason0'] ];
	document.getElementById("initial_heap").textContent = values['init_heap_size']+' bytes';
	document.getElementById("current_heap").textContent = values['current_heap_size']+' bytes';
	document.getElementById("largest_heap_block").textContent = values['largest_free_heap_block']+' bytes';

	document.getElementById("ota_board").textContent = values['ota_board'];
	document.getElementById("ota_device").textContent = values['ota_device'];
	document.getElementById("ota_config").textContent = values['ota_config'];
	let str;
	switch( values['ota_code'] ) {
		case -3:
			str = 'Update available';
			break;
		case -2:
			str = 'No update profile';
			break;
		case -1:
			str = 'No update available';
			break;
		case 0:
			str = 'Ok';
			break;
		case 1:
			str = 'Network error (HTTP)';
			break;
		case 2:
			str = 'Write error';
			break;
		case 3:
			str = 'Profile error';
			break;
		case 4:
			str = 'Profile Failed';
			break;
		default:
			str = 'Unknown';
			break;
	}
	document.getElementById("ota_status").textContent = str;

	document.getElementById("gps_fix").textContent = ( values['gps_fix'] ) ? 'Yes':'No';
	document.getElementById("gps_fix").style = ( values['gps_fix'] ) ? 'color:green':'color:red';
	document.getElementById("gps_longitude").textContent = values['gps_longitude'];
	document.getElementById("gps_latitude").textContent = values['gps_latitude'];
	document.getElementById("gps_altitude").textContent = values['gps_altitude'];

	let date = new Date( values['gps_time_sec']*1000 + values['gps_time_usec'] );
	let iso = /(\d{4}-\d{2}-\d{2})T(\d{2}:\d{2}:\d{2}.\d{3})/.exec(date.toISOString());
	document.getElementById("gps_time").textContent = iso[1]+' '+iso[2];

	document.getElementById("dome_shutter_status").textContent = DOME_SHUTTER_STATUS[ values['shutter_status'] ];
	document.getElementById("dome_shutter_moving").textContent = values['shutter_moving'];
	document.getElementById("dome_shutter_closed").textContent = values['shutter_closed'];
	document.getElementById("dome_shutter_close").textContent = values['shutter_close'];

	document.getElementById("temperature").textContent = values['temperature'].toFixed(2);
	document.getElementById("dewpoint").textContent = values['dew_point'].toFixed(2);
	document.getElementById("pressure").textContent = values['pressure'].toFixed(2);
	document.getElementById("rh").textContent = values['rh'].toFixed(2);
	document.getElementById("windspeed").textContent = values['wind_speed'].toFixed(2);
	document.getElementById("windgust").textContent = values['wind_gust'].toFixed(2);
	document.getElementById("winddirection").textContent = ( values['wind_direction'] >= 0 ) ? wind_direction[ values['wind_direction'] ]: 'Unavailable' ;

	document.getElementById("msas").textContent = values['msas'].toFixed(2);
	document.getElementById("nelm").textContent = values['nelm'].toFixed(2);

	document.getElementById("illuminance").textContent = values['lux'].toFixed(2);
	document.getElementById("irradiance").textContent = values['irradiance'].toFixed(2);

	document.getElementById("rainintensity").textContent = values['rain_intensity'];

	if ( values['available_sensors'] & BME_SENSOR ) {
		document.getElementById('temp_led').querySelector('.led_color').style.backgroundColor = 'green';
		document.getElementById('pres_led').querySelector('.led_color').style.backgroundColor = 'green';
		document.getElementById('rh_led').querySelector('.led_color').style.backgroundColor = 'green';
	} else {
		document.getElementById('temp_led').querySelector('.led_color').style.backgroundColor = 'red';
		document.getElementById('pres_led').querySelector('.led_color').style.backgroundColor = 'red';
		document.getElementById('rh_led').querySelector('.led_color').style.backgroundColor = 'red';
	}

	if ( values['available_sensors'] & WS_SENSOR )
		document.getElementById('ws_led').querySelector('.led_color').style.backgroundColor = 'green';
	else
		document.getElementById('ws_led').querySelector('.led_color').style.backgroundColor = 'red';

	if ( values['available_sensors'] & WV_SENSOR )
		document.getElementById('wv_led').querySelector('.led_color').style.backgroundColor = 'green';
	else
		document.getElementById('wv_led').querySelector('.led_color').style.backgroundColor = 'red';

	if ( values['available_sensors'] & RAIN_SENSOR )
		document.getElementById('rain_led').querySelector('.led_color').style.backgroundColor = 'green';
	else
		document.getElementById('rain_led').querySelector('.led_color').style.backgroundColor = 'red';

	if ( values['available_sensors'] & TSL_SENSOR ) {
		document.getElementById('tsl_led').querySelector('.led_color').style.backgroundColor = 'green';
		document.getElementById('sqm_led').querySelector('.led_color').style.backgroundColor = 'green';
	} else {
		document.getElementById('tsl_led').querySelector('.led_color').style.backgroundColor = 'red';
		document.getElementById('sqm_led').querySelector('.led_color').style.backgroundColor = 'red';
	}
}
