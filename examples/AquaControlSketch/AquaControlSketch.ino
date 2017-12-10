#include <OneWire.h>
#include <Arduino.h>
#include <AquaControl.h>
#include <SPI.h>
#include <SD.h>

AquaControl aqc;

void setup() {
	// put your setup code here, to run once:
	Serial.begin(19200);

	// Init AquaControl
	aqc.init();		
}

void loop() {

	aqc.proceedCycle();
	  
	// OLD LOOP FOR TESTING
	if (Serial.available()){
		String sInput = Serial.readStringUntil('\n');

		Serial.println(sInput);
		switch (sInput[0]){
		case 'm':
			Serial.println(F("Switching to manual mode"));
			break;
		case 'a':
			Serial.println(F("Switching to automatic mode"));
			break;
		default:
			break;
		}
	}
	//digitalClockDisplay();
}

void digitalClockDisplay(){
	// digital clock display of the time
	Serial.print(hour());
	printDigits(minute());
	printDigits(second());
	Serial.print(" ");
	Serial.print(day());
	Serial.print(" ");
	Serial.print(month());
	Serial.print(" ");
	Serial.print(year());
	Serial.println();
}

void printDigits(int digits){
	// utility function for digital clock display: prints preceding colon and leading 0
	Serial.print(":");
	if (digits < 10)
		Serial.print('0');
	Serial.print(digits);
}