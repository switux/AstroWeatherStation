/*
	alpaca_observingconditions.h

	ASCOM ALPACA Server for the AstroWeatherStation (c) 2023 F.Lesage

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

#pragma once
#ifndef _ALPACA_OBSERVINGCONDITIONS_H
#define _ALPACA_OBSERVINGCONDITIONS_H

#define	OBSERVINGCONDITIONS_SUPPORTED_ACTIONS	""
#define OBSERVINGCONDITIONS_DRIVER_INFO			"OpenAstroDevices AWS Environmental and SQM sensors driver / v1.0.0"
#define OBSERVINGCONDITIONS_DESCRIPTION			"OpenAstroDevices AWS Environmental and SQM sensors / v1.0.0"
#define OBSERVINGCONDITIONS_NAME				"OpenAstroDevices AWS Environmental and SQM sensors driver / v1.0.0"
#define OBSERVINGCONDITIONS_DRIVER_VERSION		"1.0"
#define	OBSERVINGCONDITIONS_INTERFACE_VERSION	1

// Attempt to convert RG-9 rain scale to ASCOM's ...
const float rain_rate[] = {
	0.F,			// No rain
	0.1F,			// Rain drops
	1.0F,			// Very light
	2.0F,			// Light
	2.5F,			// Medium light
	5.0F,			// Medium
	10.0F,			// Heavy
	50.1F			// Violent
};

#endif
