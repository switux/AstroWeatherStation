function send_config()
{
	req = new XMLHttpRequest();
	req.open( "POST", "/set_config", true );
	req.setRequestHeader("Content-Type", "application/json;charset=UTF-8");
	req.send( JSON.stringify(Object.fromEntries( ( new FormData(document.querySelector('#config') )).entries())) );
}

function display_values()
{
	req = new XMLHttpRequest();
	req2 = new XMLHttpRequest();
	req2.onreadystatechange = function() {
		if ( this.readyState == 4 && this.status == 200 ) {
			document.getElementById("root_ca").value = req2.responseText;
		}
	};
	req.onreadystatechange = function() {
		if ( this.readyState == 4 && this.status == 200 ) {
			values = JSON.parse( req.responseText );
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
			document.getElementById("tzname").value = values['tzname'];
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
			document.getElementById("has_bme").checked = values['has_bme'];
			document.getElementById("has_tsl").checked = values['has_tsl'];
			document.getElementById("has_mlx").checked = values['has_mlx'];
			document.getElementById("has_ws").checked = values['has_ws'];
			document.getElementById("has_wv").checked = values['has_wv'];
			document.getElementById("has_rg9").checked = values['has_rg9'];
			document.getElementById("has_gps").checked = values['has_gps'];

			document.querySelector("#anemometer_model").value=values['anemometer_model'];
			document.querySelector("#windvane_model").value=values['windvane_model'];
			
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
			document.getElementById("show_wifi_ap_password").style.display = "none";
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
			document.getElementById("show_wifi_sta_password").style.display = "none";
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
	document.getElementById("show_wifi_ap_ip").style.display = "table-row";
	document.getElementById("show_wifi_ap_gw").style.display = "table-row";
	document.getElementById("show_wifi_ap_dns").style.display = "table-row";
	document.getElementById("show_eth_ip_mode").style.display = "none";
	document.getElementById("show_eth_ip").style.display = "none";
	document.getElementById("show_eth_gw").style.display = "none";
	document.getElementById("show_eth_dns").style.display = "none";
}