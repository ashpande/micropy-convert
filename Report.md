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

## Development Process:

We use Clang to dump the AST to the terminal. Initially, there are a lot of unresolved expressions as Arduino keywords cannot be parsed by Clang. So we include all the Arduino core files in one folder, along with all the relevant AVR-libC headers that Arduino requires. This is because we want the AST, and not the hex dump. After editing the headers to resolve dependency issues and some definitions (such as renaming macros to const variables so that they can be seen by the compiler), we get an AST as shown below:

![AST](https://github.com/AshutoshPandey123456/micropy-convert/blob/master/Proposal%20Images/Clang-AST.png)

Then we write a FrontEndAction along with a RecursiveASTVisitor to visit our nodes recursively. We also define a ASTConsumer interface. There is some boilerplate code that has to be only defined once:

![BoilerPlate](https://github.com/AshutoshPandey123456/micropy-convert/blob/master/Proposal%20Images/ASTConsumer.png)

Then we identify a function, statement, or variable that we want to alter. We see its representation in the AST, and use clang query along with [the AST matcher Reference](https://clang.llvm.org/docs/LibASTMatchersReference.html). Using this reference and our knowledge of the AST, we write custom matchers with patterns (and sometimes, antipatterns) to narrow our query down to a specific node. Most of the functions are defined as CallExpr or DeclRefExpr and can be traced to their declarations in the headers via Callee() and hasAncestor() functions. Other standard C style expressions such as isAlpha(), isAscii() are recorded as ShadowDecl nodes, so we use that. The matchers are then binded with a keyword.

![clang-query](https://github.com/AshutoshPandey123456/micropy-convert/blob/master/Proposal%20Images/clang-query.png)

![Matchers](https://github.com/AshutoshPandey123456/micropy-convert/blob/master/Proposal%20Images/matcher.png)

Next, we define a Handler Class for each node. Again, there is some boilerplate code which is repeated every time we want to access a node. We use the rewrite class to make precise edits on the AST, transforming them to the equivalent MicroPython expressions. We can also include warnings for transformations that need specific modules to work.

![Handler](https://github.com/AshutoshPandey123456/micropy-convert/blob/master/Proposal%20Images/Handler.png)

We rebuild the tool after saving it, using the ninja keyword in the build directory:

![ClangToolBuild](https://github.com/AshutoshPandey123456/micropy-convert/blob/master/Proposal%20Images/clangtoolbuild.png)

Using this process, almost any part of the code can be accessed and edited.

## Advantages of this Approach:

- The nodes can be made very specific, so syntactically incorrect code will not be converted.

- The same technique can be used to develop a CPP checker.

- Key words only in a specific node are edited, leaving out the rest.

- Code that is not transformable is left as is, so that the user can edit it out.

- Can include error and diagnostic messages.

## Future Improvements:

- Currently the Serial class is broken and cannot be accessed by the Clang AST, this needs to be fixed by editing the headers and dependencies. Clang has AVR support but its not full fledged.

- Arduino's MicroPython port is in its infancy, and as hardware specific implementations become more clear, the tool will develop to accomodate that.
