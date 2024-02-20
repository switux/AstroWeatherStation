function config_section_active( section_name, yes_no )
{
	if ( yes_no == true ) {

		document.getElementById( section_name ).style.display = 'flex';
		document.getElementById( section_name ).style.backgroundColor = '#f6f6f6';
		document.getElementById( "banner_"+section_name ).style.backgroundColor = "#f6f6f6";

	} else {

		document.getElementById( section_name ).style.display = 'none';
		document.getElementById( section_name ).style.backgroundColor = '#dbdbdb';
		document.getElementById( "banner_"+section_name ).style.backgroundColor = "#dbdbdb";

	}
}

function toggle_panel( panel_id )
{
	switch( panel_id ) {

		case 0:
			config_section_active( "general", true );
			config_section_active( "network", false );
			config_section_active( "sensors", false );
			config_section_active( "lookout", false );
			break;

		case 1:
			config_section_active( "general", false );
			config_section_active( "network", true );
			config_section_active( "sensors", false );
			config_section_active( "lookout", false );
			break;

		case 2:
			config_section_active( "general", false );
			config_section_active( "network", false );
			config_section_active( "sensors", true );
			config_section_active( "lookout", false );
			break;

		case 3:
			config_section_active( "general", false );
			config_section_active( "network", false );
			config_section_active( "sensors", false );
			config_section_active( "lookout", true );
			break;

	}
}

function send_config()
{
	let req = new XMLHttpRequest();
	req.open( "POST", "/set_config", true );
	req.setRequestHeader( "Content-Type", "application/json;charset=UTF-8" );
	req.send( JSON.stringify(Object.fromEntries( ( new FormData(document.querySelector('#config') )).entries())) );
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
	console.log( values['lookout_enabled'] );
	
	document.getElementById("lookout_enabled").checked = ( values['lookout_enabled'] ==  '1' )? 'true' : 'false';

	fill_lookout_value( "unsafe_wind_speed_1", false, true, values );
	fill_lookout_value( "unsafe_wind_speed_2", false, true, values );
	fill_lookout_value( "unsafe_cloud_coverage_1", true, true, values );
	fill_lookout_value( "unsafe_cloud_coverage_2", true, true, values );
	fill_lookout_value( "unsafe_rain_intensity", true, true, values );

	fill_lookout_value( "safe_wind_speed_active", false, false, values );
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

function display_values()
{
	toggle_panel( 0 );
	let req = new XMLHttpRequest();
	let req2 = new XMLHttpRequest();
	req2.onreadystatechange = function() {
		if ( this.readyState == 4 && this.status == 200 ) {
			document.getElementById("root_ca").value = req2.responseText;
		}
	};
	req.onreadystatechange = function() {

		if ( this.readyState == 4 && this.status == 200 ) {

			let values = JSON.parse( req.responseText );
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