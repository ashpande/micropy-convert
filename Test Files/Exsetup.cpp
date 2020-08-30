#include "Arduino.h"

int inPin = 7;
void setup() {

  Serial.begin(9600);
}

void loop() {
   int val = digitalRead(inPin);
      digitalWrite(10, HIGH);
	pinMode( 13, INPUT);
	pinMode( 12, OUTPUT);
	pinMode( 11, INPUT_PULLUP);
        char thischar = 'c';
}
