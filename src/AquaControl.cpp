/*
Aqua Control Library

Creationdate: 2016-12-28
Created by Marcel Schulz (Schullebernd)

Further information on www.schullebernd.de

Copyright 2016
*/

#include "AquaControl.h"
#if defined(USE_NTP)
#include <WiFiUdp.h>
unsigned int localPort = 2390;      // local port to listen for UDP packets
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP udp;
#endif

#if defined(USE_WEBSERVER)
ESP8266WebServer _Server(80);
#endif

AquaControl* _aqc;
uint8_t mac[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 };

#if defined(ESP8266)
void AquaControl::initESP8266NetworkConnection(){
	Serial.print(F("Connecting to "));
	Serial.print(_WlanConfig.SSID);
	WiFi.persistent(false);
	WiFi.mode(WIFI_STA);
	if (_WlanConfig.ManualIP){
		Serial.print(F(" using fixed IP "));
		Serial.print(_WlanConfig.IP.toString());
		WiFi.config(_WlanConfig.IP, _WlanConfig.Gateway, IPAddress(255, 255, 255, 0));
	}
	WiFi.begin(_WlanConfig.SSID.c_str(), _WlanConfig.PW.c_str());
	uint8_t iTimeout = 20; // 10 sec should be enough for connecting to an existing wlan
	while (WiFi.status() != WL_CONNECTED && iTimeout > 0) {
		delay(500);
		Serial.print(".");
		iTimeout--;
	}
	if (iTimeout == 0) {
		Serial.print(F(" Timeout. Switching to standard AP Mode. Please connect to WiFi SSID 'SBAQC_WIFI' using password 'sbaqc12345'."));
		WiFi.softAPdisconnect();
		WiFi.disconnect();
		WiFi.mode(WIFI_AP);
		WiFi.softAPConfig(IPAddress(192, 168, 0, 1), IPAddress(192, 168, 0, 1), IPAddress(255, 255, 255, 0));
		WiFi.softAP("SBAQC_WIFI", "sbaqc12345");
	}
	else {
		Serial.println(F(" Done."));
	}
	Serial.print(F("IP address: "));
	Serial.println(WiFi.localIP());
}

Option extractOptionFromConfigLine(String sLine){
	Option option;
	option.Key = sLine.substring(0, sLine.indexOf('='));
	String sValue = sLine.substring(sLine.indexOf('=') + 1);
	// Filter leading spaces at the beginnning of the config line
	while (sLine.length() > 0 && sLine.charAt(0) == ' '){
		sLine = sLine.substring(1);
	}
	if (sValue.length() > 2 && sValue.charAt(0) == '"' && sValue.charAt(sValue.length() -1) == '"'){
		option.Value = sValue.substring(1, sValue.length() - 1);
	}
	else{
		option.Value = "";
	}
	return option;
}

String buildLine(Option opt){
	return opt.Key + "=\"" + opt.Value + "\"";
}

bool AquaControl::writeWlanConfig(){
	File wlanCfg = SD.open("config/wlan.cfg");
	if (SD.exists("config/wlan_new.cfg")){
		if (!SD.remove("config/wlan_new.cfg")){
			Serial.println(F("Error deleting old wlan_new.cfg"));
			return false;
		}
		else{
			Serial.println(F("wlan_new.cfg deleted."));
		}
	}
	if (!wlanCfg){
		Serial.println(F("Error opening config/wlan.cfg"));
	}
	File wlanCfgNew = SD.open("config/wlan_new.cfg", FILE_WRITE);
	if (!wlanCfgNew){
		Serial.println(F("Error creating config/wlan_new.cfg"));
		return false;
	}

	if (wlanCfg && wlanCfgNew){
		while (wlanCfg.available()){
			String sLine = wlanCfg.readStringUntil(10); // Read until line feed char(10)
			// Filter carriage return (13) from the end of the line
			if (sLine.charAt(sLine.length() - 1) == 13){
				sLine = sLine.substring(0, sLine.length() - 1);
			}
			// Filter tailing spaces at the end of the config line
			while (sLine.length() > 2 && sLine.charAt(sLine.length() - 1) == ' '){
				sLine = sLine.substring(0, sLine.length() - 1);
			}

			// Search for config settings
			if (sLine.startsWith("mode")){
				// mode setting found
				Serial.println(F("Mode line found"));
				Serial.print(F("Old line: "));
				Serial.println(sLine);
				Option opt;
				opt.Key = "mode";
				opt.Value = _WlanConfig.Mode == WlanModeClient ? "client" : "ap";
				sLine = buildLine(opt);
				Serial.print(F("New line: "));
				Serial.println(sLine);
			}
			if (sLine.startsWith("ssid")){
				// ssid setting found
				Serial.println(F("SSID line found"));
				Serial.print(F("Old line: "));
				Serial.println(sLine);
				Option opt;
				opt.Key = "ssid";
				opt.Value = _WlanConfig.SSID;
				sLine = buildLine(opt);
				Serial.print(F("New line: "));
				Serial.println(sLine);
			}
			if (sLine.startsWith("pw")){
				// password setting found
				Serial.println(F("PW line found"));
				Serial.print(F("Old line: "));
				Serial.println(sLine);
				Option opt;
				opt.Key = "pw";
				opt.Value = _WlanConfig.PW;
				sLine = buildLine(opt);
				Serial.print(F("New line: "));
				Serial.println(sLine);
			}
			if (sLine.startsWith("ip")){
				// ip address setting found
				Serial.println(F("IP line found"));
				Serial.print(F("Old line: "));
				Serial.println(sLine);
				Option opt;
				opt.Key = "ip";
				opt.Value = _WlanConfig.ManualIP ? _WlanConfig.IP.toString() : "auto";
				sLine = buildLine(opt);
				Serial.print(F("New line: "));
				Serial.println(sLine);
			}
			if (sLine.startsWith("gateway")){
				// ip address setting found
				Serial.println(F("Gateway line found"));
				Serial.print(F("Old line: "));
				Serial.println(sLine);
				Option opt;
				opt.Key = "gateway";
				opt.Value = _WlanConfig.ManualIP ? _WlanConfig.Gateway.toString() : "auto";
				sLine = buildLine(opt);
				Serial.print(F("New line: "));
				Serial.println(sLine);
			}
			wlanCfgNew.write((sLine + (char)13 + char(10)).c_str());
		}
		wlanCfg.close();
		wlanCfgNew.close();
		SD.remove("config/wlan.cfg");
		wlanCfg = SD.open("config/wlan.cfg", FILE_WRITE);
		wlanCfgNew = SD.open("config/wlan_new.cfg");
		while (wlanCfgNew.available()){
			String sLine = wlanCfgNew.readStringUntil(10);
			if (sLine.charAt(sLine.length() - 1) == 13){
				sLine = sLine.substring(0, sLine.length() - 1);
			}
			wlanCfg.write((sLine + (char)13 + char(10)).c_str());
		}
		wlanCfg.close();
		wlanCfgNew.close();
		if (!SD.remove("config/wlan_new.cfg")){
			Serial.println("Error deleting new wlan_new.cfg");
			return false;
		}
		else{
			Serial.println("wlan_new.cfg deleted.");
		}
		return true;
	}
	else{
		Serial.println("Error opening files");
		return false;
	}
}

bool AquaControl::readWlanConfig(){
	File wlanCfg = SD.open("config/wlan.cfg");
	String sMode;
	String sSSID;
	String sPW;
	String sIP;
	String sGateway;

	if (wlanCfg) {
		// Read line by line
		while (wlanCfg.available()){
			String sLine = wlanCfg.readStringUntil(10);	
			if (sLine.charAt(sLine.length() - 1) == 13){
				sLine = sLine.substring(0, sLine.length() - 1);
			}
			// Filter leading spaces at the beginnning of the config line
			while (sLine.length() > 0 && sLine.charAt(0) == ' '){
				sLine = sLine.substring(1);
			}
			// Filter tailing spaces at the end of the config line
			while (sLine.length() > 2 && sLine.charAt(sLine.length() - 1) == ' '){
				sLine = sLine.substring(0, sLine.length() - 1);
			}
			// Search for config settings
			if (sLine.startsWith("mode")){
				// mode setting found
				sMode = extractOptionFromConfigLine(sLine).Value;
				sMode.toLowerCase();
			}
			if (sLine.startsWith("ssid")){
				// ssid setting found
				sSSID = extractOptionFromConfigLine(sLine).Value;
			}
			if (sLine.startsWith("pw")){
				// password setting found
				sPW = extractOptionFromConfigLine(sLine).Value;
			}
			if (sLine.startsWith("ip")){
				// ip address setting found
				sIP = extractOptionFromConfigLine(sLine).Value;
			}
			if (sLine.startsWith("gateway")){
				// ip address setting found
				sGateway = extractOptionFromConfigLine(sLine).Value;
			}
		}
		// Currently we have to fix this to client mode
		sMode = "client";
		this->_WlanConfig.SSID = sSSID;
		this->_WlanConfig.PW = sPW;
		if (sIP != "auto"){
			this->_WlanConfig.ManualIP = true;
			this->_WlanConfig.IP = extractIPAddress(sIP);
		}
		else{
			this->_WlanConfig.ManualIP = false;
			this->_WlanConfig.IP = IPAddress((uint32_t)0);
		}
		if (sGateway != "auto"){
			this->_WlanConfig.Gateway = extractIPAddress(sGateway);
		}
		else{
			this->_WlanConfig.Gateway = IPAddress((uint32_t)0);
		}

		// close the file:
		wlanCfg.close();
		return true;
	}
	else{
		// if the file didn't open, print an error:
		Serial.println("error opening wlan.cfg");
	}
}
#endif

bool AquaControl::readLedConfig(){
	// Iterate through the pwm channels
	for (uint8_t i = 0; i < PWM_CHANNELS; i++){
		char sTempName[30]; // This is used because the string objects causing freezes when using in SD.exists or SD.open commands
		String sPwmFilename = "config/ledch_";
		sPwmFilename += (i <= 9 ? "0" : "") + String(i);
		sPwmFilename += ".cfg";
		// Delete the old config file for the channel
		sPwmFilename.toCharArray(sTempName, 30);
		if (!SD.exists(sTempName)){
			Serial.println(String(F("Warning: Couldn't find config file for LED channel ")) + String(i+1));
			continue;
		}
		else{
			File pwmFile = SD.open(sTempName);
			if (!pwmFile){
				Serial.println(String(F("Error: Couln't open config file for LED channel ")) + String(i+1));
				continue;
			}
			else{
				while (pwmFile.available()){
					// First line is always the time
					String sLine = pwmFile.readStringUntil(10);
					if (sLine.charAt(sLine.length() - 1) == 13){
						sLine = sLine.substring(0, sLine.length() - 1);
					}
					// Filter leading spaces at the beginnning of the config line
					while (sLine.length() > 0 && sLine.charAt(0) == ' '){
						sLine = sLine.substring(1);
					}
					// Filter tailing spaces at the end of the config line
					while (sLine.length() > 2 && sLine.charAt(sLine.length() - 1) == ' '){
						sLine = sLine.substring(0, sLine.length() - 1);
					}
					if (sLine.length() == 0){
						// Ignore this line
						continue;
					} else if (sLine.startsWith("//")){
						// Irgnoe this line too
						continue;
					}
					// Try to separate time and value 
					uint8_t iSemikolonIndex = sLine.indexOf(';');
					String sTime = sLine.substring(0, iSemikolonIndex);
					String sValue = sLine.substring(iSemikolonIndex + 1);
					// PArse the time
					int8_t index = sTime.indexOf(':');
					long targetTime = 0;
					// Eighter in format of hh:mm
					if (index != -1){
						int8_t hour = sLine.substring(0, index).toInt();
						int8_t min = sLine.substring(index + 1).toInt();
						targetTime = (60 * 60 * hour) + (60 * min);
					}
					else{ // Or in format of seconds of the day
						targetTime = sLine.toInt();
					}
					if (targetTime > (60 * 60 * 24)){
						// if the time is longer than a day, so put it to the last second in a day
						targetTime = 3600 * 24;
					}
					Target target;
					target.Time = targetTime;
					target.Value = sValue.toInt();
					_PwmChannels[i].addTarget(target);
				}
			}
		}
	}
}

bool AquaControl::writeLedConfig(uint8_t pwmChannel){
	char sTempFilename[30]; // This for SD.exists and SD.open because the SD library freezes when using String objects
	String sPwmFilename = "config/ledch_";
	sPwmFilename += pwmChannel <= 9 ? (String("0") + String(pwmChannel)) : String(pwmChannel);
	sPwmFilename += ".cfg";
	sPwmFilename.toCharArray(sTempFilename, 30);
	// Delete the old config file for the channel
	if (SD.exists(sTempFilename)){
		if (!SD.remove(sTempFilename)){
			Serial.println(String(F("Error: Couldn't remove old config file ")) + sPwmFilename);
			return false;
		}
	}
	// Create the new config file
	File pwmFile = SD.open(sTempFilename, FILE_WRITE);
	if (!pwmFile){
		Serial.println(String(F("Error: Couldn't create config file ")) + sPwmFilename);
		return false;
	}
	// Iterate trough the targets and write them to the file
	// the format is Time;Vlaue (e.g. 08:30;100)
	for (uint8_t t = 0; t < _PwmChannels[pwmChannel].TargetCount; t++){
		Target tTarget = _PwmChannels[pwmChannel].Targets[t];
		// Write the time in hh:mm format
		String sHour, sMinute;
		uint8_t iHour = hour(tTarget.Time);
		uint8_t iMinute = minute(tTarget.Time);
		pwmFile.write((String((iHour > 9 ? "" : "0")) + String(iHour) + ":" + String((iMinute > 9 ? "" : "0")) + String(iMinute)).c_str());
		pwmFile.write(';');
		// Write the value
		pwmFile.write(String(tTarget.Value).c_str());
		// Write a new line
		pwmFile.write(13);
		pwmFile.write(10);
	}
	pwmFile.close();
	return true;
}

void AquaControl::initTimeKeeper(){
#if defined(USE_RTC_DS3231)
	Serial.print(F("Initializing RTC DS3231..."));
	long l = now();
	do {
		Serial.print(".");
		setSyncProvider(RTC.get);   // the function to get the time from the RTC
		if (timeStatus() != timeSet){
			delay(500);
			l = now();
		}
	} while (timeStatus() != timeSet && l < 10);

	if (timeStatus() != timeSet){
		Serial.println(F(" Failed: Unable to sync with the RTC"));
	}
	else {
		Serial.println(F(" Done."));
	}
#elif defined(USE_NTP)
#error "Not yet implemented"
#endif
}

uint8_t AquaControl::getPhysicalChannelAddress(uint8_t channelNumber){
	switch (channelNumber){
	case 0:
		return PWM_CHANNEL_0;
	case 1:
		return PWM_CHANNEL_1;
#if defined(USE_PCA9685) || defined( __AVR_ATmega2560__)
	case 2:
		return PWM_CHANNEL_2;
	case 3:
		return PWM_CHANNEL_3;
	case 4:
		return PWM_CHANNEL_4;
	case 5:
		return PWM_CHANNEL_5;
	case 6:
		return PWM_CHANNEL_6;
	case 7:
		return PWM_CHANNEL_7;
	case 8:
		return PWM_CHANNEL_8;
	case 9:
		return PWM_CHANNEL_9;
	case 10:
		return PWM_CHANNEL_10;
	case 11:
		return PWM_CHANNEL_11;
	case 12:
		return PWM_CHANNEL_12;
	case 13:
		return PWM_CHANNEL_13;
	case 14:
		return PWM_CHANNEL_14;
	case 15:
		return PWM_CHANNEL_15;
#endif
	default:
		return -1;
	}
}

void AquaControl::init(){

	// Init SD card device
	Serial.println();
	Serial.println(F("Schullebernd Aqua Control"));
	Serial.println(F("-------------------------"));
	Serial.print(F("(Version ")); Serial.print(AQC_BUILD); Serial.println(F(")"));
	Serial.println(F("Now starting up"));
	_aqc = this;
	Serial.print(F("Initializing SD card..."));
	if (!SD.begin(SD_CS)) {
		Serial.println(F(" Failed"));
		return;
	}
	Serial.println(F(" Done."));

#if defined(ESP8266)
	Serial.print(F("Reading wlan config from SD card..."));
	if (!readWlanConfig()){
		Serial.println(F(" Failed"));
		return;
	}
	Serial.println(F(" Done."));

	initESP8266NetworkConnection();
#endif
	// Init the time mechanism (RTC or NTP)
	initTimeKeeper();
	CurrentSecOfDay = elapsedSecsToday(now());
	CurrentMilli = nowMs();

	// Init the pwm channels
	Serial.print(F("Initializing PWM channels..."));
	for (uint8_t i = 0; i < PWM_CHANNELS; i++){
		_PwmChannels[i].ChannelAddress = getPhysicalChannelAddress(i);
	}
	Serial.println(" Done.");

#if defined(USE_WEBSERVER)
	Serial.print(F("Initializing Webserver..."));
	_Server = ESP8266WebServer(80);
	_Server.on("/", handleRoot);
	_Server.on("/editled", HTTP_GET, handleEditLedGET);
	_Server.on("/editled", HTTP_POST, handleEditLedPOST);
	_Server.on("/editwlan", HTTP_GET, handleEditWlanGET);
	_Server.on("/editwlan", HTTP_POST, handleEditWlanPOST);
	_Server.on("/edittime", HTTP_GET, handleTimeGET);
	_Server.on("/edittime", HTTP_POST, handleTimePOST);
	_Server.on("/test", HTTP_GET, handleTestModeGET);
	_Server.on("/test", HTTP_POST, handleTestModePOST);
	_Server.on("/css/style.css", HTTP_GET, handleStyleGET);

	_Server.onNotFound(handleNotFound);
	_Server.begin();
	Serial.println(F(" Done."));
#else
	Serial.println(F("Webserver is deactivated."));
#endif

#if defined(USE_PCA9685)
	// Initialize the PCA9685 pwm / servo driver
	Serial.print(F("Initializing PCA9685 module..."));
	pwm.begin();
	pwm.setPWMFreq(PWM_FREQ);
	Serial.println(F(" Done."));
#endif

	Serial.println(F("Reading LED config from SD card..."));
	if (readLedConfig()){
		Serial.println(F(" Done."));
	}

#if defined(USE_DS18B20_TEMP_SENSOR)
	Serial.print(F("Initializing DS18B20 Temerature Sensor..."));
	if (!_Temperature.init(CurrentSecOfDay)){
		Serial.println(F(" Failed"));
	}
	else {
		Serial.println(F("Done."));
	}
#endif
	Serial.println(F("AQC booting completed."));
}

bool AquaControl::addChannelTarget(uint8_t channel, Target target){
	if (channel > PWM_CHANNELS){
		return false;
	}
	else{
		uint8_t pos = _PwmChannels[channel].addTarget(target);
		Serial.print(F("Added target at position "));
		Serial.print(pos);
		Serial.print(F(" of channel "));
		Serial.println(channel);
	}
}

void AquaControl::proceedCycle() {
	uint8_t cycle = 0;
	CurrentSecOfDay = elapsedSecsToday(now());
	CurrentMilli = nowMs();

	for (cycle; cycle < PWM_CHANNELS; cycle++) {
		_PwmChannels[cycle].proceedCycle(CurrentSecOfDay, CurrentMilli);
		if (_PwmChannels[cycle].HasToWritePwm || _IsFirstCycle) {
			writePwmToDevice(cycle);
		}
	}
	_IsFirstCycle = false;

#if defined(USE_WEBSERVER)
	// Hande the Webserver features
	_Server.handleClient();
#endif

#if defined(USE_DS18B20_TEMP_SENSOR)
	if (_Temperature.Status) {
		_Temperature.readTemperature(_PwmChannels->CurrentSecOfDay);
	}
	else {

	}
#endif
}

#if defined(USE_DS18B20_TEMP_SENSOR)
bool TemperatureReader::readTemperature(time_t currentSeconds) {
	// Check, if we can do any activity
	if (currentSeconds >= _NextPossibleActivity || currentSeconds < (_NextPossibleActivity - 120)){ // The second test is for the case of day change
	// Is it a new reading or the second stage of reading?
		if (!_TickTock) {
			_TickTock = true;
			// It a new reading
			temp.reset();
			temp.select(temp_addr);
			temp.write(0x44, 1);        // start conversion, with parasite power on at the end

			// Thedelay is not nessessary because we implement the daly by wayting for the next possible activity

			_NextPossibleActivity = currentSeconds + 2;
			// delay(1000);     // maybe 750ms is enough, maybe not
						 // we might do a ds.depower() here, but the reset will take care of it.
			return false;
		}
		else {
			_TickTock = false;
			// We are in the second stage of a reading, so lets read the value from the sensor
			temp_present = temp.reset();
			temp.select(temp_addr);
			temp.write(0xBE);         // Read Scratchpad

			for (uint8_t i = 0; i < 9; i++) {           // we need 9 bytes
				temp_data[i] = temp.read();
			}

			// Convert the data to actual temperature
			// because the result is a 16 bit signed integer, it should
			// be stored to an "int16_t" type, which is always 16 bits
			// even when compiled on a 32 bit processor.
			int16_t raw = (temp_data[1] << 8) | temp_data[0];
			if (temp_type_s) {
				raw = raw << 3; // 9 bit resolution default
				if (temp_data[7] == 0x10) {
					// "count remain" gives full 12 bit resolution
					raw = (raw & 0xFFF0) + 12 - temp_data[6];
				}
			}
			else {
				byte cfg = (temp_data[4] & 0x60);
				// at lower res, the low bits are undefined, so let's zero them
				if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
				else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
				else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
													  //// default is 12 bit resolution, 750 ms conversion time
			}
			_TemperatureInCelsius = (float)raw / 16.0;
			Serial.print(F("  Temperature = "));
			Serial.print(_TemperatureInCelsius);
			Serial.println(F(" Celsius."));
			_NextPossibleActivity = currentSeconds + _UpdateIntervall + 1;
			return true;
		}
	}
	return false;
}

bool TemperatureReader::init(time_t currentSecOfDay) {
	_NextPossibleActivity = currentSecOfDay;
	if (!temp.search(temp_addr)) {
		temp.reset_search();
		delay(250);
		Status = false;
		return false;
	}
	if (OneWire::crc8(temp_addr, 7) != temp_addr[7]) {
		Status = false;
		return false;
	}
	Status = true;
	return true;
}
#endif

void AquaControl::writePwmToDevice(uint8_t channel){
#if defined(USE_PCA9685)
	pwm.setPWM(_PwmChannels[channel].ChannelAddress, 0, _PwmChannels[channel].CurrentWriteValue);
#else
	analogWrite(_PwmChannels[channel].ChannelAddress, _PwmChannels[channel].CurrentWriteValue);
#endif
}

#if defined(ESP8266)
IPAddress AquaControl::extractIPAddress(String sIP){
	//Serial.println(sIP);
	uint32_t ip = 0;
	int8_t iIndex = 0;
	while ((iIndex = sIP.lastIndexOf(".")) != -1){
		uint8_t singleVal = sIP.substring(iIndex + 1).toInt();
		ip |= singleVal;
		ip = ip << 8;
		//Serial.println(ip, BIN);
		// remove the leading dot
		sIP = sIP.substring(0, iIndex);
	}
	ip |= sIP.substring(0).toInt();
	return IPAddress(ip);
}
#endif

uint8_t PwmChannel::addTarget(Target t){
	
	if (TargetCount >= MAX_TARGET_COUNT_PER_CHANNEL){
		return -1;
	}
	else{
		time_t newTargetTime = elapsedSecsToday(t.Time);
		for (uint8_t i = 0; i < TargetCount - 1; i++){
			time_t currentTime = elapsedSecsToday(Targets[i].Time);
			if (newTargetTime < currentTime){
				// We have to put in the new target before the current one to keep the right time order
				// First move all following targets one slot to right
				for (uint8_t n = TargetCount; n > i; n--){
					Targets[n] = Targets[n - 1];
				}
				// Now insert the new target
				Targets[i] = t;
				TargetCount++;
				return i;
			}
		}
		// If no target was inserted, the the new target must be placed at the end of the list
		Targets[TargetCount] = t;
		TargetCount++;
		return TargetCount;
	}
}

bool PwmChannel::removeTargetAt(uint8_t pos){
	if (pos >= TargetCount){
		return false;
	}
	else if (pos == (TargetCount - 1)){
		TargetCount--;
		return true;
	}
	else{
		for (uint8_t i = pos; i < (TargetCount - 1); i++){
			Targets[i] = Targets[i + 1];
		}
		TargetCount--;
		return true;
	}
}

#define PWM_MIN 1
void PwmChannel::proceedCycle(time_t currentSecOfDay, time_t currentMilliOfSec){
	if (TargetCount > 0){
		HasToWritePwm = false;
		CurrentSecOfDay = currentSecOfDay;
		CurrentMilli = currentMilliOfSec;

		Target currentTarget;
		Target lastTarget;
		bool bTargetFound = false;
		
		// if only one target is set
		if (TargetCount == 1){
			// then we have a constant value from 00:00:00 until 23:59:59
			currentTarget.Time = (60 * 60 * 24); // End of the day
			currentTarget.Value = lastTarget.Value = Targets[0].Value; // Take the constant value
			lastTarget.Time = 0; // start of day
		} else {
			// find the current and last target
			for (uint8_t t = 0; t < TargetCount; t++){
				if (Targets[t].Time > CurrentSecOfDay){
					currentTarget = Targets[t];
					bTargetFound = true;
					// if it is the first timer event of the day
					if (t == 0){
						// then take the value from the last timer in the pool and calculate the relative time of the day
						lastTarget.Time = 0 - ((60 * 60 * 24) - Targets[TargetCount - 1].Time);
						lastTarget.Value = Targets[TargetCount - 1].Value;
					}
					else{
						// simply take the previous target
						lastTarget = Targets[t - 1];
					}
					break;
				}
			}
			// if no target was found (current time is later than the last target in this day)
			if (!bTargetFound){
				// take the first target of the next day (simply the first target in the pool)
				currentTarget = Targets[0];
				// take the last target of the current day
				lastTarget = Targets[TargetCount - 1];
				// and now correct the time, because the new target is at the next day. So we have to add the remaining time of the current day
				currentTarget.Time = currentTarget.Time + (60 * 60 * 24) - lastTarget.Time;
			}
		}
		

		// now calculate the graph between the two target values
		unsigned long dt = currentTarget.Time - lastTarget.Time;
		int16_t dv = currentTarget.Value - lastTarget.Value;
		float m = ((float)dv) / ((float)dt) / 1000.0;
		float n = ((float)lastTarget.Value); // -(m * ((float)0.0));
		float deltaNow = ((float)(CurrentSecOfDay - lastTarget.Time) * 1000.0) + (float)CurrentMilli;
		float vx = (m * deltaNow) + n;
		if (m > 0.0){
			if (vx > (float)currentTarget.Value){
				vx = currentTarget.Value;
			}
		}
		else{
			if (vx < (float)currentTarget.Value){
				vx = currentTarget.Value;
			}
		}
		// The testmode is integrated here because we only overwrite the current _PwmTargetValue.
		// Also this makes sense, because we do not influence the complete process of calculation and setting of the light values.
		if (TestMode) {
			_PwmTarget = (uint16_t)(((float)PWM_MAX * TestValue) / 100.0);
			if (TestModeSetTime < (_aqc->CurrentSecOfDay - 60) || TestModeSetTime > _aqc->CurrentSecOfDay) {
				TestMode = false;
			}
		}
		else {
			_PwmTarget = (uint16_t)(((float)PWM_MAX * vx) / 100.0);
		}
		
		// Try to fade to the target value and do not jump
		// If you wish to jump, then set PMW_STEP bigger than the pwm precicion
		if (_PwmTarget != _PwmValue){
			HasToWritePwm = true;
			if (_PwmTarget > _PwmValue){
				_PwmValue += PWM_STEP;
				if (_PwmTarget > _PwmValue + 100){
					_PwmValue += PWM_STEP;
				}
				if (_PwmValue > _PwmTarget){
					_PwmValue = _PwmTarget;
					//Serial.print("_PwmTarget reached at ");
					//Serial.println(_PwmTarget);
				}
			}
			else{
				_PwmValue -= PWM_STEP;
				if (_PwmTarget < _PwmValue - 100){
					_PwmValue -= PWM_STEP;
				}
				if (_PwmValue < _PwmTarget){
					_PwmValue = _PwmTarget;
					//Serial.print("_PwmTarget reached at ");
					//Serial.println(_PwmTarget);
				}
			}
			CurrentWriteValue = _PwmValue;
			// Thins defines a minimum light value
			if (CurrentWriteValue > 0 && CurrentWriteValue < PWM_MIN){
				CurrentWriteValue = PWM_MIN;
			}
		}
	}
 else{
	 _PwmTarget = 0;
	 _PwmValue = 0;
	 CurrentWriteValue = 0;
 }
}

