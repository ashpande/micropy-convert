# Arduino to MicroPython Transcompiler

## Supported conversions (As of 31/08/2020)

### I/O: Digital, Analog and Advanced:

- digitalRead(inPin) to Pin.value(inPin)

- digitalWrite(pin, value) to Pin.value(pin, value)

- pinMode(pin, INPUT) to pin.Mode(p(number).IN)

- pinMode(pin, OUTPUT) to pin.Mode(p(number).OUT)

- pinMode(pin, PULLUP) to pin.Mode(p(number).PULL_UP)

- analogRead(analogpin) to ADC.read_u16(analogpin)

- analogWrite(pin, value) to machine.PWM(pin, value)

- pulseIn(pin, value) to machine.time_pulse_us(pin, value)

Note: some of these statements also insert a comment asking the user to import machine module.

### Time:

- delay(ms) to utime.sleep_ms(ms)

- delayMicroseconds(us) to utime.sleep_us(us)

- millis() to utime.ticks_ms()

- micros() to utime.ticks_us()

### Math:

- pow(base, exponent) to math.pow(base, exponent)

- sqrt(x) to math.sqrt(x)

- cos(rad) to math.cos(rad)

- sin(rad) to math.sin(rad)

- tan(rad) to math.tan(rad)

- Macro PI = 3.14159... to math.pi

- Macro EULER = 2.7... to math.e

### Characters:

- isAlpha(thisChar) to ure.match('[A-Za-z]', String)

- isAlphaNumeric(thisChar) to ure.match('[A-Za-z0-9]', String)

- isAscii(thisChar) to ure.match('\w\W', String)

- isDigit(thisChar) to ure.match('\d', String)

- isLowerCase(thisChar) to ure.match('[a-z]', String)

- isPunct(thisChar) to ure.match('\W', String)

- isSpace(thisChar) to ure.match('\f\n\r\t\v\s', String)

- isUpperCase(thisChar) to ure.match('[A-Z]', String)

- isWhitespace(thisChar) to ure.match('\s\t', String)

Note: Wherever these expressions occur in the sketch, the tool gives a warning to import ure

###  Sketch:

- void loop() to While True:

- void setup() is deleted, MicroPython sketches don't have void setup()

- {} are commented out with a '#' since Python does not use curly braces for scope. They are not deleted however, since a user converting a badly indented sketch might lose scope information. So we comment out the {} for every expression where it is a compound statement.

- If() and for() are currently detected by the tool, the if and else part are expanded with comments. More features relating to this will be released in upcoming versions.
