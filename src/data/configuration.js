function toggle_panel( panel_id )
{
	switch( panel_id ) {

		case 0:
			document.getElementById("general").style.display = 'flex';
			document.getElementById("general").style.backgroundColor = '#f6f6f6';
			document.getElementById("network").style.display = 'none';
			document.getElementById("network").style.backgroundColor = '#dbdbdb';
			document.getElementById("sensors").style.display = 'none';
			document.getElementById("sensors").style.backgroundColor = '#dbdbdb';
			document.getElementById("lookout").style.display = 'none';
			document.getElementById("lookout").style.backgroundColor = '#dbdbdb';
			document.getElementById("banner_general").style.backgroundColor = "#f6f6f6";
			document.getElementById("banner_network").style.backgroundColor = "#dbdbdb";
			document.getElementById("banner_sensors").style.backgroundColor = "#dbdbdb";
			document.getElementById("banner_controls").style.backgroundColor = "#dbdbdb";
			
			break;

		case 1:
			document.getElementById("general").style.display = 'none';
			document.getElementById("general").style.backgroundColor = '#dbdbdb';
			document.getElementById("network").style.display = 'flex';
			document.getElementById("network").style.backgroundColor = '#f6f6f6';
			document.getElementById("sensors").style.display = 'none';
			document.getElementById("sensors").style.backgroundColor = '#dbdbdb';
			document.getElementById("lookout").style.display = 'none';
			document.getElementById("lookout").style.backgroundColor = '#dbdbdb';
			document.getElementById("banner_network").style.backgroundColor = "#f6f6f6";
			document.getElementById("banner_general").style.backgroundColor = "#dbdbdb";
			document.getElementById("banner_sensors").style.backgroundColor = "#dbdbdb";
			document.getElementById("banner_controls").style.backgroundColor = "#dbdbdb";
			break;

		case 2:
			document.getElementById("general").style.display = 'none';
			document.getElementById("general").style.backgroundColor = '#dbdbdb';
			document.getElementById("network").style.display = 'none';
			document.getElementById("network").style.backgroundColor = '#dbdbdb';
			document.getElementById("sensors").style.display = 'flex';
			document.getElementById("sensors").style.backgroundColor = '#f6f6f6';
			document.getElementById("lookout").style.display = 'none';
			document.getElementById("lookout").style.backgroundColor = '#dbdbdb';
			document.getElementById("banner_sensors").style.backgroundColor = "#f6f6f6";
			document.getElementById("banner_network").style.backgroundColor = "#dbdbdb";
			document.getElementById("banner_general").style.backgroundColor = "#dbdbdb";
			document.getElementById("banner_controls").style.backgroundColor = "#dbdbdb";
			break;

		case 3:
			document.getElementById("general").style.display = 'none';
			document.getElementById("general").style.backgroundColor = '#dbdbdb';
			document.getElementById("network").style.display = 'none';
			document.getElementById("network").style.backgroundColor = '#dbdbdb';
			document.getElementById("sensors").style.display = 'none';
			document.getElementById("sensors").style.backgroundColor = '#dbdbdb';
			document.getElementById("lookout").style.display = 'flex';
			document.getElementById("lookout").style.backgroundColor = '#f6f6f6';
			document.getElementById("banner_sensors").style.backgroundColor = "#dbdbdb";
			document.getElementById("banner_network").style.backgroundColor = "#dbdbdb";
			document.getElementById("banner_general").style.backgroundColor = "#dbdbdb";
			document.getElementById("banner_controls").style.backgroundColor = "#f6f6f6";
			break;

	}
}

function send_config()
{
	var req = new XMLHttpRequest();
	req.open( "POST", "/set_config", true );
	req.setRequestHeader("Content-Type", "application/json;charset=UTF-8");
	req.send( JSON.stringify(Object.fromEntries( ( new FormData(document.querySelector('#config') )).entries())) );
}

function fill_lookout_values( values )
{
	console.log( values['lookout_enabled'] );
	
	document.getElementById("lookout_enabled").checked = ( values['lookout_enabled'] ==  '1' )? 'true' : 'false';

	document.getElementById("unsafe_wind_speed_active_1").checked = ( values['unsafe_wind_speed_active_1'] == "1" )? 'true' : 'false';
	document.getElementById("unsafe_wind_speed_max_1").value = values['unsafe_wind_speed_max_1'];
	document.getElementById("unsafe_wind_speed_delay_1").value = values['unsafe_wind_speed_delay_1'];
	document.getElementById("unsafe_wind_speed_active_2").checked = ( values['unsafe_wind_speed_active_2'] == "1" )? 'true' : 'false';
	document.getElementById("unsafe_wind_speed_max_2").value = values['unsafe_wind_speed_max_2'];
	document.getElementById("unsafe_wind_speed_delay_2").value = values['unsafe_wind_speed_delay_2'];
	document.getElementById("unsafe_cloud_coverage_active_1").checked = ( values['unsafe_cloud_coverage_active_1'] == "1" )? 'true' : 'false';
	document.querySelector("#unsafe_cloud_coverage_max_1").value = values['unsafe_cloud_coverage_max_1'];
	document.getElementById("unsafe_cloud_coverage_delay_1").value = values['unsafe_cloud_coverage_delay_1'];
	document.getElementById("unsafe_cloud_coverage_active_2").checked = ( values['unsafe_cloud_coverage_active_2'] == "1" )? 'true' : 'false';
	document.querySelector("#unsafe_cloud_coverage_max_2").value = values['unsafe_cloud_coverage_max_2'];
	document.getElementById("unsafe_cloud_coverage_delay_2").value = values['unsafe_cloud_coverage_delay_2'];
	document.getElementById("unsafe_rain_intensity_active").checked = ( values['unsafe_rain_intensity_active'] == "1" )? 'true' : 'false';
	document.querySelector("#unsafe_rain_intensity_max").value = values['unsafe_rain_intensity_max'];

	document.getElementById("safe_wind_speed_active").checked = ( values['safe_wind_speed_active'] == "1" )? 'true' : 'false';
	document.getElementById("safe_wind_speed_max").value = values['safe_wind_speed_max'];
	document.getElementById("safe_wind_speed_delay").value = values['safe_wind_speed_delay'];
	document.getElementById("safe_cloud_coverage_active_1").checked = ( values['safe_cloud_coverage_active_1'] == "1" )? 'true' : 'false';
	document.querySelector("#safe_cloud_coverage_max_1").value = values['safe_cloud_coverage_max_1'];
	document.getElementById("safe_cloud_coverage_delay_1").value = values['safe_cloud_coverage_delay_1'];
	document.getElementById("safe_cloud_coverage_active_2").checked = ( values['safe_cloud_coverage_active_2'] == "1" )? 'true' : 'false';
	document.querySelector("#safe_cloud_coverage_max_2").value = values['safe_cloud_coverage_max_2'];
	document.getElementById("safe_cloud_coverage_delay_2").value = values['safe_cloud_coverage_delay_2'];
	document.getElementById("safe_rain_intensity_active").checked = ( values['safe_rain_intensity_active'] == "1" )? 'true' : 'false';
	document.getElementById("safe_rain_intensity_delay").value = values['safe_rain_intensity_delay'];
	document.querySelector("#safe_rain_intensity_max").value = values['safe_rain_intensity_max'];

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

function display_values()
{
	toggle_panel( 0 );
	req = new XMLHttpRequest();
	var req2 = new XMLHttpRequest();
	req2.onreadystatechange = function() {
		if ( this.readyState == 4 && this.status == 200 ) {
			document.getElementById("root_ca").value = req2.responseText;
		}
	};
	req.onreadystatechange = function() {

    if ( this.readyState == 4 && this.status == 200 ) {

			var values = JSON.parse( req.responseText );
			document.getElementById("tzname").value = values['tzname'];
			fill_network_values( values );
			fill_sensor_values( values );
			fill_lookout_values( values );
			console.log( values );

		}
	};
	req.open( "GET", "/get_config", true );
	req.send();
	req2.open( "GET", "/get_root_ca", true );
	req2.send();
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

function toggle_lookout()
{
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