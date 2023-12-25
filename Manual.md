# !! WIP DOCUMENT !!

# Configuration

You can amend your default configuration by changing data/aws.conf.dfl which contains the following JSON string:

{"pcb_version":"3.x","pwr_mode":2,"has_ethernet":1,"has_sc16is750":1,"pref_iface":2,"alpaca_iface":2,"config_iface":2}

# Configuration reference
## Power supply mode
- 0: Solar panel
- 1: 12V DC
- 2: PoE
## PCB version
 - "1.x"
 - "2.x"
 - "3.x"

## Interface numbering
- 0 : WIFI AP
- 1: WIFI STA
- 2: Ethernet

## IP Mode
- 0: DHCP
- 1: Manual

## Configuration items
- **eth_ip_mode**: 0, 1
  - Ethernet IP configuration
- **eth_ip**: xxx.xxx.xxx.xxx/CIDR mask
  - Ethernet IP address, overriden of **eth_ip_mode** is DHCP. CIDR mask value. e.g.: 24 for a 255.255.255.0 (see [Wikipedia article](https://en.wikipedia.org/wiki/Classless_Inter-Domain_Routing#IPv4_CIDR_blocks "Wikipedia article") for more information)
- **pcb_version**:
  - Indicates the hardware/PCB design if the station, you should not change the default value unless you understand the implications
- **pwr_mode**: 
  - Sets the type of power supply, when set to "Solar panel", the station will sleep most of the time and wake up at fixed intervals to update sensor values and send them to a server (see **url_path**  and  **remote_server** parameters )
- **has_ethernet**: 1 or 0
  - Activates ethernet support, only supported with pcb_version >= 3.x
- **has_sc16is750**: 1 or 0
  - Activates SC16IS750 UART/GPIO extender, only supported with pcb_version >= 3.x
- **pref_iface**: 0, 1, 2
  - Preferred interface to connect to the station.
- **alpaca_iface**: 0, 1, 2
  - Default interface of the ALPACA server.
- **config_iface**: 0,1,2
  - Default interface of the configuration server.
