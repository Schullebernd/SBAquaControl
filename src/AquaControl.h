/*
Aqua Control Library

Creationdate: 2016-12-28
Created by Marcel Schulz (Schullebernd)

Further information on www.schullebernd.de

Copyright 2016
*/

#ifndef __AQUACONTROL_H_
#define __AQUACONTROL_H_

#include "AquaControl_config.h"

#define AQC_VERSION "0.5"
#define AQC_BUILD	"0.5.001"

#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#endif

#if defined(USE_RTC_DS3231)
#include <Wire.h>
#include <DS3232RTC.h>
#endif

#if defined(USE_DS18B20_TEMP_SENSOR)
#include <OneWire.h>
#define DS18B20_PIN	D4 // Defines the pin, where the data wire of the DS18B20 sensor is connected
#endif

#if defined(USE_PCA9685)
#include <Adafruit_PWMServoDriver.h>
#define PWM_FREQ 300
#endif

// This is for the sd card modul
#include <SPI.h>
#include <SD.h>


#include <TimeLib.h>

#if defined(USE_WEBSERVER)
// Handles for the Webserer
void handleRoot();
void handleEditWlanGET();
void handleEditWlanPOST();
void handleEditLedPOST();
void handleEditLedGET();
void handleNotFound();
void handleTestModeGET();
void handleTestModePOST();
void handleTimeGET();
void handleTimePOST();
void handleStyleGET();
#endif

#if defined (__AVR__)
#define SD_CS 0
#elif defined(ESP8266)
#define SD_CS D8
#else
#define SD_CS TODO
#endif

#if defined(USE_PCA9685)
	#define PWM_STEP 5
	#define PWM_MAX 4095 // 12 bit PCA9685 Board
	/* Defines the maximum number of supported pwm channels. This is restriced by the PCA9685 controller */
	#define PWM_CHANNELS 16
	#define PWM_CHANNEL_0	0
	#define PWM_CHANNEL_1	1
	#define PWM_CHANNEL_2	2
	#define PWM_CHANNEL_3	3
	#define PWM_CHANNEL_4	4
	#define PWM_CHANNEL_5	5
	#define PWM_CHANNEL_6	6
	#define PWM_CHANNEL_7	7
	#define PWM_CHANNEL_8	8
	#define PWM_CHANNEL_9	9
	#define PWM_CHANNEL_10	10
	#define PWM_CHANNEL_11	11
	#define PWM_CHANNEL_12	12
	#define PWM_CHANNEL_13	13
	#define PWM_CHANNEL_14	14
	#define PWM_CHANNEL_15	15
#elif defined(__AVR__)// defined(USE_PCA9685)
	#define PWM_STEP 1
	#define PWM_MAX 255		// AVR or Arduino has only 8 bit PWM output
	#define PWM_CHANNEL_0	5
	#define PWM_CHANNEL_1	9
	#define PWM_CHANNEL_2	3
	#define PWM_CHANNEL_3	6
	#define PWM_CHANNEL_4	10
	#define PWM_CHANNEL_5	11
	#if defined(__AVR_ATmega2560__)
		/* Defines the maximum number of supported pwm channels. This is restriced by the PCA9685 controller */
		#define PWM_CHANNELS 16
		#define PWM_CHANNEL_6	4
		#define PWM_CHANNEL_7	2
		#define PWM_CHANNEL_8	7
		#define PWM_CHANNEL_9	8
		#define PWM_CHANNEL_10	12
		#define PWM_CHANNEL_11	13
		#define PWM_CHANNEL_12	44
		#define PWM_CHANNEL_13	45
		#define PWM_CHANNEL_14	46
		#define PWM_CHANNEL_15	47
	#else
		/* Defines the maximum number of supported pwm channels. This is restriced by the PCA9685 controller */
		#define PWM_CHANNELS 6
	#endif // defined(__AVR_ATmega2560__)
#elif defined (ESP8266)
	#define PWM_STEP 4
	#define PWM_MAX 1023	// ESP8266 has software PWM output with 10 bit precision
	#define PWM_CHANNEL_0	D0
	#define PWM_CHANNEL_1	D3
	/* Defines the maximum number of supported pwm channels. This is restriced by the PCA9685 controller */
	#define PWM_CHANNELS 2
#else	// elif defined(ESP8266)
	#define PWM_STEP 1
	#define PWM_MAX 255		// Asume a 8 bit PWM for all other cpu types
	/* Defines the maximum number of supported pwm channels. This is restriced by the PCA9685 controller */
	#define PWM_CHANNELS 4
	#define PWM_CHANNEL_0	FUNC_GPIO0
	#define PWM_CHANNEL_1	FUNC_GPIO1
	#define PWM_CHANNEL_2	FUNC_GPIO2
	#define PWM_CHANNEL_3	FUNC_GPIO3
#endif	// defined(__AVR__)

typedef struct{
	String Key;
	String Value;
}Option;

#if defined(ESP8266)
enum WlanMode{
	WlanModeClient,
	WlanModeAccessPoint
};

typedef struct{
	WlanMode	Mode;
	String		SSID;
	String		PW;
	bool		ManualIP;
	IPAddress	IP;
	IPAddress	Gateway;
}WlanConfig;
#endif

/*This struct defines a target value at a specific time */
typedef struct{
	uint8_t		Value;			// Percentage value must between 0 and 100
	time_t		Time;			// The time in seconds after 01.01.1970 when the value should be reached.
}Target;

class PwmChannel{
private:
	int16_t		_PwmTarget = 0;
	int16_t		_PwmValue = 1;

public:
	uint8_t		ChannelAddress; // Contains the address or pin for setting the pwm value
	Target		Targets[MAX_TARGET_COUNT_PER_CHANNEL];
	uint8_t		TargetCount;
	uint16_t	CurrentWriteValue;
	bool		HasToWritePwm; // Indecates, that a new pwm values has to be written to the pwm device
	bool		TestMode;
	time_t		TestModeSetTime;
	uint8_t		TestValue;
	time_t		CurrentSecOfDay;
	time_t		CurrentMilli;

	PwmChannel(){
		TestMode = false;
	}

	uint8_t addTarget(Target t); // Inserts a new target (time and value for the channel) and gives back the position.

	bool removeTargetAt(uint8_t pos); // Removes the target at the specified position

	void proceedCycle(time_t currentSecOfDay, time_t currentMilliOfSec); // the main function for each step. Here the pwm value will be calculated
};

#if defined(USE_DS18B20_TEMP_SENSOR)

class TemperatureReader {
public:
	OneWire temp = OneWire(DS18B20_PIN);
	bool Status = false;
	byte temp_addr[8];
	byte temp_data[12];
	byte temp_type_s;
	// Stores the current temperature
	float _TemperatureInCelsius;
	byte temp_present = 0;
	// This is used in the temperature read funktion. To read a temperature value from the sensor takes at least one second.
	// But we can not wait for, because the cycle time in AquaControl has to be as low as possible. Thats why we will use a ticktock princip.
	// To  read a temperatur value we have to call the temp read function twice. The first call prepares the sensor. The second call reads the value.
	// Between the calls at least one second has to leave.
	bool _TickTock = false;
	time_t _NextPossibleActivity;
	uint8_t _UpdateIntervall = 10;


	// Reads the temperature from the DS18B20 sensor and stores it to the member. The function decides by it self, if a temperatur value will we red.
	// So, do not think, that a call of the function will update the _TemperatureInCelcius member for shure.
	// The function returns true, if the temperature value was updated
	bool readTemperature(time_t currentSeconds);
	bool init(time_t currentSecOfDay);
};

#endif


class AquaControl{
public:
	time_t CurrentSecOfDay;
	time_t CurrentMilli;

#if defined (USE_PCA9685)
	Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
#endif

#if defined (USE_DS18B20_TEMP_SENSOR)
	TemperatureReader _Temperature;
	uint8_t _TemperatureUpdateIntervall = 10; // Update the temperatur every 10 sec
#endif

#if defined(ESP8266)
	// Initializes the Wifi-Network 
	void initESP8266NetworkConnection();
	// Read ant write the Wlan configuration
	bool readWlanConfig();
	bool writeWlanConfig();
	// Read and write led configuration
	bool readLedConfig();
	bool writeLedConfig(uint8_t pwmChannel);
#endif

	// Initializes the time synch mechanisim (RTC or NTP)
	void initTimeKeeper();	

	uint8_t getPhysicalChannelAddress(uint8_t channelNumber);

	PwmChannel	_PwmChannels[PWM_CHANNELS];	// Stores the PWM chanels
	bool		_IsFirstCycle;				// Indicates, that we have not set any pwm value
#if defined(ESP8266)
	WlanConfig	_WlanConfig;
#endif

	AquaControl(){
		_IsFirstCycle = true;
	}

	void init();

	bool addChannelTarget(uint8_t channel, Target target);

	void proceedCycle();

	void writePwmToDevice(uint8_t channel);

#if defined(ESP8266)
	IPAddress extractIPAddress(String sIP);
#endif

};

#endif //#ifndef __AQUACONTROL_H_
