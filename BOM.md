# Bill of materials (WIP)

## Enclosures

- Station: 1x IP67 150x210x47
- Environmental sensor: 1x MISOL Shield (B016Q7LHP2)
- Other sensors: 1x IP67 65x58x35
- Solar panel: 1x 3mm PVC sheet (50x25) (not for POE and 12V versions)

## Boards

- 1x ESP32 WROOM 32U (DevKit C), 38 pins
- 2x MAX485
- 1x SC16IS750 UART/GPIO module
- 1x W5500 mini (for POE version, optional for 12V version)

## Wifi (optional for POE and 12V versions)

- 1x 5dBi external antenna
- 1x SMA male connector 
- 1x IPEX to RP-SMA pigtail

## Ethernet (POE version, and 12V optional)

- 1x W5500 Mini module

## PCB

- 1x 8x12cm universal PCB double side

## Connectors (12V & POE versions)

- 8x KF2510 2pins			(2x buttons, 1x relay pwr, 1x dome status, 1x dome control, 1x 12V wind sensor, 1x 12V RG-9, 1x 12V pwr)
- 1x KF2510 4pins			(I2C)
- 1x KF2510 5pins			(RG9)
- 2x Single row female header 19pins	(ESP32)
- 2x Single row female header 11pins	(SC16IS750)
- 2x Single row female header 5pins	(W5500)
- 5x Single row female header 4pins	(GPS,MAX485)
- 4x Single row female header 2pins	(Mini560)

## Cables

- 1x IP67 micro USB cable male to female, 30cm
- 1.5m 22AWG UL2464 4 cores
- 1.5m 24AWG single core
- 1m 22AWG UL2464 7 cores
- 0.5m 20AWG UL2464 2 cores
- 0.5m 22AWG

## Sensors

- 1x TLS2591
- 1x MLX90614 BAA
- 1x BME280
- 1x RS485 Wind vane
- 1x RS485 Anemometer
- 1x Hydreon RG-9
- 1x Neo-8M/Neo-6M GPS

## Power

- 1x Mini560 step down 3.3V
- 1x DD04CVSA 12V (not for POE and 12V versions)
- 2x LiPo battery 103450 (not for POE and 12V versions)
- 1x Solar panel 6V 3W (not for POE and 12V versions)
- 1x 48V POE PD (for IPTV camera) 12V2A (only for POE version)

## Small parts -- electronics

- 4x 0.25W 10k resistor
- 1x 0.25W 82k resistor (only for solar panel version)
- 1x 0.25W 300k resistor (only for solar panel version)
- 3x 0.25W 1M resistor (only for solar panel version)

- 1x 10uF electrolytic capacitor
- 3x IRZN44L (only for solar panel version)
- 1x LM 7805 (only for solar panel version)
- 1x SN74AHCT125 (only for solar panel version)

## Small parts -- mechanics

- 8x PG7 cable gland
- 8x M2.6 8mm screws
- 1x IP55 Push button red (reboot)
- 1x IP55 Push button black (debug/config)
- 1deg FoV LED clear lens with holder (diam. 13mm)
