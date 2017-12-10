/*
Aqua Control Library

Creationdate: 2017-12-09
Created by Marcel Schulz (Schullebernd)

Further information on www.schullebernd.de

Copyright 2017
*/

#include "AquaControl.h"

#if defined(USE_WEBSERVER)

extern "C" ESP8266WebServer _Server;
extern "C" AquaControl* _aqc;

void handleRoot() {
	File myFile = SD.open(F("index.htm"));
	if (myFile) {

		_Server.setContentLength(CONTENT_LENGTH_UNKNOWN);
		_Server.send(200, "text/html", "");
		while (myFile.available()) {
			String sLine = myFile.readStringUntil(10);
			sLine.replace("##FW_VERSION##", AQC_BUILD);
#if defined(USE_DS18B20_TEMP_SENSOR)
			sLine.replace("##TEMP##", "Aktuelle Wassertemperatur " + String(_aqc->_Temperature._TemperatureInCelsius) + " &deg;C<br/>");
#else
			sLine.replace("##TEMP##", "");
#endif
			_Server.sendContent(sLine);
		}

		// close the file:
		myFile.close();
	}
	else {
		// if the file didn't open, print an error:
		Serial.println(F("error opening editled.htm"));
	}
}

void handleEditWlanGET() {
	File myFile = SD.open("wlan.htm");
	if (myFile) {

		_Server.setContentLength(CONTENT_LENGTH_UNKNOWN);
		_Server.send(200, "text/html", "");
		while (myFile.available()) {
			String sLine = myFile.readStringUntil(10);
			if (sLine.indexOf("##WLAN_CONFIG##") != -1) {
				String sContent = "<tr><td>SSID</td><td><input type=\"text\" name=\"ssid\" value=\"" + _aqc->_WlanConfig.SSID + "\"/></td></tr>\n";
				sContent += "<tr><td>Passwort</td><td><input type=\"password\" name=\"password\" value=\"\"/></td></tr>\n";
				sContent += "<tr><td>IP-Adresse</td><td><input type=\"text\" name=\"ip\" value=\"" + (_aqc->_WlanConfig.ManualIP ? _aqc->_WlanConfig.IP.toString() : "") + "\"/></td></tr>\n";
				sContent += "<tr><td>Gateway</td><td><input type=\"text\" name=\"gateway\" value=\"" + (_aqc->_WlanConfig.ManualIP ? _aqc->_WlanConfig.Gateway.toString() : "") + "\"/></td></tr>\n";
				_Server.sendContent(sContent);
			}
			else {
				sLine.replace("##FW_VERSION##", AQC_BUILD);
				_Server.sendContent(sLine);
			}
		}

		// close the file:
		myFile.close();
	}
	else {
		// if the file didn't open, print an error:
		Serial.println(F("error opening editled.htm"));
	}
}

void handleEditWlanPOST() {
	String sSSID = _Server.arg("ssid");
	Serial.println(sSSID);
	String sPw = _Server.arg("password");
	Serial.println(sPw);
	String sIP = _Server.arg("ip");
	Serial.println(sIP);
	String sGateway = _Server.arg("gateway");
	Serial.println(sGateway);
	if (sSSID.length() > 0) {
		_aqc->_WlanConfig.SSID = sSSID;
		if (sPw != "") {
			_aqc->_WlanConfig.PW = sPw;
		}
		_aqc->_WlanConfig.IP = _aqc->extractIPAddress(sIP);
		if (sIP.length() > 6) {
			_aqc->_WlanConfig.IP = _aqc->extractIPAddress(sIP);
			_aqc->_WlanConfig.ManualIP = true;
		}
		else {
			_aqc->_WlanConfig.IP = IPAddress((uint32_t)0);
			_aqc->_WlanConfig.ManualIP = false;
		}

		if (sGateway.length() > 6) {
			_aqc->_WlanConfig.Gateway = _aqc->extractIPAddress(sGateway);
		}
		else {
			_aqc->_WlanConfig.Gateway = IPAddress((uint32_t)0);
		}
		_aqc->writeWlanConfig();
	}
	handleEditWlanGET();
}

void handleEditLedPOST() {
	// Save button in edit led channel was pressed
	String channel = _Server.arg("channel");
	uint8_t iChannel = atoi(channel.c_str());

	Target targets[MAX_TARGET_COUNT_PER_CHANNEL];
	uint8_t targetCount = 0;
	for (uint8_t i = 0; i < _Server.args(); i++) {
		String sArg = _Server.argName(i);
		if (sArg.startsWith("tt")) {
			// we have a target time value
			int8_t targetNumber = sArg.substring(2).toInt();
			// get the time of the target
			String sTargetTime = _Server.arg(sArg);
			// if it is a valid target time
			if (sTargetTime.length() > 0) {
				int8_t index = sTargetTime.indexOf(':');
				long targetTime = 0;
				if (index != -1) {
					int8_t hour = sTargetTime.substring(0, index).toInt();
					int8_t min = sTargetTime.substring(index + 1).toInt();
					targetTime = (60 * 60 * hour) + (60 * min);
				}
				else {
					targetTime = sTargetTime.toInt();
				}
				if (targetTime > (60 * 60 * 24)) {
					// the time is longer than a day, so put it to the last second in a day
					targetTime = 3600 * 24;
				}

				// now get the appropriate value for the target
				String sValueArgName = "tv";
				sValueArgName += sArg.substring(2);
				uint8_t targetValue = _Server.arg(sValueArgName).toInt();

				// correct possible wrong inputs
				if (targetValue > 100) {
					targetValue = 100;
				}
				targets[targetCount].Time = targetTime;
				targets[targetCount].Value = targetValue;
				targetCount++;
			}
			else {
				/*Serial.print("cancel input ");
				Serial.print(sArg);
				Serial.print(" time value ");
				Serial.println(sTargetTime);*/
			}
		}
	}
	Serial.print(F("Removing old targets..."));
	for (uint8_t t = 0; t < MAX_TARGET_COUNT_PER_CHANNEL; t++) {
		_aqc->_PwmChannels[iChannel].removeTargetAt(0);
	}
	Serial.println(F(" Done."));
	Serial.print(F("Inserting new targets... "));
	for (uint8_t t = 0; t < targetCount; t++) {
		/*Serial.print("Add target time ");
		Serial.print(targets[t].Time);
		Serial.print(" value ");
		Serial.println(targets[t].Value);*/
		_aqc->_PwmChannels[iChannel].addTarget(targets[t]);
	}
	Serial.println(F(" Done."));
	Serial.print(F("Storing new led config to SD card..."));
	_aqc->writeLedConfig(iChannel);
	Serial.println(F(" Done."));
	_aqc->_IsFirstCycle = true;

	handleEditLedGET();
}

void handleEditLedGET() {
	File myFile = SD.open("editled.htm");
	if (myFile) {
		String channel = _Server.arg("channel");
		uint8_t iChannel = atoi(channel.c_str());

		_Server.setContentLength(CONTENT_LENGTH_UNKNOWN);
		_Server.send(200, "text/html", "");
		while (myFile.available()) {
			String sLine = myFile.readStringUntil('\n');
			if (sLine.indexOf("##CONTENT##") != -1) {
				for (uint8_t i = 0; i < MAX_TARGET_COUNT_PER_CHANNEL; i++) {
					String sContent = "";
					if (i < _aqc->_PwmChannels[iChannel].TargetCount) {
						String sHour, sMinute;
						uint8_t iHour = hour(_aqc->_PwmChannels[iChannel].Targets[i].Time);
						uint8_t iMinute = minute(_aqc->_PwmChannels[iChannel].Targets[i].Time);
						sContent += "<tr><td>";
						sContent += i + 1;
						sContent += "</td><td> <input type = \"text\" value=\"";
						sContent += String((iHour > 9 ? "" : "0")) + String(iHour) + ":" + String((iMinute > 9 ? "" : "0")) + String(iMinute);
						sContent += "\" name=\"tt";
						sContent += (i > 9 ? "" : "0") + String(i);
						sContent += "\"/></td>";
						_Server.sendContent(sContent);
						sContent = "<td> <input type = \"text\" value=\"";
						sContent += String(_aqc->_PwmChannels[iChannel].Targets[i].Value);
						sContent += "\" name=\"tv";
						sContent += (i > 9 ? "" : "0") + String(i);
						sContent += "\"/> </td></tr>\n";
						_Server.sendContent(sContent);
					}
					else {
						sContent += "<tr><td>";
						sContent += i + 1;
						sContent += "</td><td> <input type = \"text\" value=\"";
						sContent += "";
						sContent += "\" name=\"tt";
						sContent += (i > 9 ? "" : "0") + String(i);
						sContent += "\"/></td><td><input type=\"text\" value=\"";
						sContent += "";
						sContent += "\" name=\"tv";
						sContent += (i > 9 ? "" : "0") + String(i);
						sContent += "\"/> </td></tr>\n";
						_Server.sendContent(sContent);
					}
				}
			}
			else {
				sLine.replace("##CHANNEL##", String(iChannel + 1));
				sLine.replace("##FW_VERSION##", AQC_BUILD);
				_Server.sendContent(sLine);
			}
		}

		// close the file:
		myFile.close();
	}
	else {
		// if the file didn't open, print an error:
		Serial.println(F("error opening editled.htm"));
	}
}

void handleNotFound() {
	String message = "File Not Found\n\n";
	message += "URI: ";
	message += _Server.uri();
	message += "\nMethod: ";
	message += (_Server.method() == HTTP_GET) ? "GET" : "POST";
	message += "\nArguments: ";
	message += _Server.args();
	message += "\n";
	for (uint8_t i = 0; i < _Server.args(); i++) {
		message += " " + _Server.argName(i) + ": " + _Server.arg(i) + "\n";
	}
	_Server.send(404, "text/plain", message);
}


void handleTestModeGET() {
	File myFile = SD.open("testmode.htm");
	if (myFile) {
		_Server.setContentLength(CONTENT_LENGTH_UNKNOWN);
		_Server.send(200, "text/html", "");
		while (myFile.available()) {
			String sLine = myFile.readStringUntil(10);
			if (sLine.indexOf("##CONTENT##") != -1) {
				for (uint8_t i = 0; i < PWM_CHANNELS; i++) {
					String sContent = "<tr><td>" + String((i+1)) + "</td><td><input type=\"text\" name=\"channel" + String(i) + "\" value=\"" + _aqc->_PwmChannels[i].TestValue + "\"/></td></tr>";
					_Server.sendContent(sContent);
				}
			}
			else {
				sLine.replace("##FW_VERSION##", AQC_BUILD);
				_Server.sendContent(sLine);
			}
		}

		// close the file:
		myFile.close();
	}
	else {
		// if the file didn't open, print an error:
		Serial.println(F("error opening testmode.htm"));
	}
}

void handleTestModePOST() {
	String sTestmode = _Server.arg("testmode");
	if (sTestmode.equals("on")) {
		for (uint8_t i = 0; i < PWM_CHANNELS; i++) {
			_aqc->_PwmChannels[i].TestMode = true;
			String sArg = "channel";
			sArg += String(i);
			String sValue = _Server.arg(sArg);
			uint8_t iValue = sValue.toInt();
			if (iValue < 0) {
				iValue = 0;
			}
			if (iValue > 100) {
				iValue = 100;
			}
			_aqc->_PwmChannels[i].TestValue = iValue;
			_aqc->_PwmChannels[i].TestModeSetTime = _aqc->CurrentSecOfDay;
		}
	}
	else {
		for (uint8_t i = 0; i < PWM_CHANNELS; i++) {
			_aqc->_PwmChannels[i].TestMode = false;
		}
	}
	handleTestModeGET();
}

void handleTimeGET() {
	File myFile = SD.open("time.htm");
	if (myFile) {

		_Server.setContentLength(CONTENT_LENGTH_UNKNOWN);
		_Server.send(200, "text/html", "");
		while (myFile.available()) {
			String sLine = myFile.readStringUntil(10);
			if (sLine.indexOf("##TIME_CONFIG##") != -1) {
				String sContent = "<tr><td>Datum</td><td><input type=\"text\" name=\"date\" value=\"";
				sContent += (day() <= 9 ? "0" : "") + String(day()) + "." + (month() <= 9 ? "0" : "") + String(month()) + "." + String(year());
				sContent += "\"/></td></tr>";

				sContent += "<tr><td>Zeit</td><td><input type=\"text\" name=\"time\" value=\"";
				sContent += (hour() <= 9 ? "0" : "") + String(hour()) + ":" + (minute() <= 9 ? "0" : "") + String(minute()) + ":" + (second() <= 9 ? "0" : "") + String(second());
				sContent += "\"/></td></tr>";
				_Server.sendContent(sContent);
			}
			else {
				sLine.replace("##FW_VERSION##", AQC_BUILD);
				_Server.sendContent(sLine);
			}
		}

		// close the file:
		myFile.close();
	}
	else {
		// if the file didn't open, print an error:
		Serial.println(F("error opening editled.htm"));
	}
}

void handleTimePOST() {
	String sDate = _Server.arg("date");
	String sTime = _Server.arg("time");
	uint16_t value;
	uint8_t pos;
	tmElements_t newTime;
	// Get the Day
	pos = sDate.indexOf(".");
	if (pos > 0) {
		newTime.Day = sDate.substring(0, pos).toInt();
		sDate = sDate.substring(pos + 1);
		// Get the month
		pos = sDate.indexOf(".");
		if (pos > 0) {
			newTime.Month = sDate.substring(0, pos).toInt();
			// Get the year
			sDate = sDate.substring(pos + 1);
			value = sDate.toInt();
			if (value > 1000) {
				newTime.Year = CalendarYrToTm(value);
			}
			else {
				newTime.Year = y2kYearToTm(value);
			}
			// Now get the hour
			pos = sTime.indexOf(":");
			if (pos > 0) {
				newTime.Hour = sTime.substring(0, pos).toInt();
				sTime = sTime.substring(pos + 1);
				// Get the minute
				pos = sTime.indexOf(":");
				if (pos > 0) {
					newTime.Minute = sTime.substring(0, pos).toInt();
					sTime = sTime.substring(pos + 1);
					// finally get the second if
					newTime.Second = sTime.toInt();
				}
				else {
					newTime.Minute = sTime.toInt();
				}
				time_t t = makeTime(newTime);
				// Now set the time to RTC and to the system
				RTC.set(t);
				setTime(t);
			}
		}
	}
	handleTimeGET();
}

void handleStyleGET() {
	File myFile = SD.open("css/style.css");
	if (myFile) {
		if (_Server.streamFile(myFile, "text/css") != myFile.size()) {
			Serial.println(F("Sent less data than expected!"));
		}

		// close the file:
		myFile.close();
	}
	else {
		// if the file didn't open, print an error:
		Serial.println(F("error opening index.htm"));
	}
}

#endif