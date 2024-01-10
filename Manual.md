# !! WIP DOCUMENT !!

# HW Variants

There are at least two PCB variants:
- Solar Panel
- 12V / PoE

The solar panel comes with a charger and a set of MOSFETs to switch on/off sensors and battery charge level reading.

The 12V/PoE does not include a charger, batteries, MOSFETs, etc. 

This is why there are two versions in the schematics folder. Here are the possible HW variants and features:

| | Ethernet | GPIO extender | GPS |
|-|----------|---------------|-----|
|Solar panel | No | No | No |
| 12V/PoE | Optional | Optional | Optional |

# HW Configuration companion app

Since a unique firmware version is shared between variants, it is necessary to store the HW features in a way that cannot be accidentally changed or altered during runtime config. Since ESP32 does not have any EPROM, a NVS solution is taken but which must not be modified by the firmware itself. This is the reason why a small configuration app has been written to allow the setting of the HW features.

_Insert GH link to the app_

The app will allow you, from --for example-- the Arduino IDE serial monitor, to enter your choices and then to restart the MCU so that you can check that the values have been correctly saved.

**You must first upload this app on the MCU before anything else.**

# Configuration

In case you plan to test the same basic config on different MCU's you can amend your default configuration by changing data/aws.conf.dfl which contains the following JSON string:

{"pref_iface":2,"alpaca_iface":2,"config_iface":2}

Then, from the arduino IDE, you can upload this file and get an immediately usable station from a "blank" MCU provided an AWS firmware has been uploaded. This is also handy when you badly screw up your config. This will remove the aws.conf file and leave you with a sane was.conf.dfl file :-)

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
