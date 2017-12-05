#ifndef _AQUACONTROL_CONFIG_H_
#define _AQUACONTROL_CONFIG_H_

/* Comment this out to use the on board pins to control the pwm channels.
   When using an ESP8266 then use a levelshifter from 3, 3V to 5V for connecting to a Meanwell LDD700L constant current supply.*/
#define USE_PCA9685

/* Define here what type of time keeper you want to use. */
#define USE_RTC_DS3231		// Synchronizes the time with a DS3231 RTC module. The standard i2c pins will be used
//#define USE_NTP			  // Not yet implemented - Connects to an NTP Server to get synchronize the time. This is only available if ESP8266 or EthernetShield is installed

/* Comment this out to not use the web interface functionality */
#define USE_WEBSERVER

/* Comment this out, if you do not have a DS18B20 temerature sensor */
#define USE_DS18B20_TEMP_SENSOR

#endif