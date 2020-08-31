# micropy-convert
A source to source compiler tool based on Clang Libtooling to convert Arduino Sketches(C/C++) to MicroPython. Developed for Arduino during Google Summer of Code 2020.

## Currently supported code transformations
- **Digital I/O:** digitalRead(), digitalWrite(), pinMode()
- **Analog I/O:** analogRead(), analogWrite()
- **Advanced I/O:** pulseIn()
- **Time:** delay(), delayMicroseconds(), micros(), millis()
- **Math:** pow(), sqrt(), cos(), sin(), tan()
- **Characters:** isAlpha(), isAlphaNumeric(), isAscii(), isDigit(), isLowerCase(), isPunct(), isSpace(), isUpperCase(), isWhitespace()
- **Constants:** INPUT, OUTPUT, INPUT_PULLUP, PI, EULER
- **Sketch:** loop(), setup(), for(), if(), curly braces {}

## Installation Instructions

To run the tool you require a full install of both Clang/LLVM and Libtooling along with the ninja build system. Any other linker will not work, and ninja is more performant. You can use the instructions on [this page](https://clang.llvm.org/docs/LibASTMatchersTutorial.html). The download+build process may take anywhere from 1.5 hours to 5+ hours depending on your computer, and may consume upto 16 GB of RAM. 

Using the -DCMAKE_BUILD_TYPE=Release flag in the CMAKE step is helpful in speeding up the process.

1.) Clone this repository.

2.) Place the micropy-convert folder inside the clang-tools-extra directory:
  
    $ cd clang-llvm/llvm-project/clang-tools-extra

3.) Place the Arduino-headerfiles folder inside the clang directory:
    
    $ cd clang-llvm/llvm-project

4.) Build the tool by running **ninja** from inside the build directory:
    
    $ cd clang-llvm/llvm-project/build
    $ ninja
    
## Using the tool

Place the file which has to be translated into the Arduino-headerfiles folder. Make sure it has a **.cpp** extension and has #include "Arduino.h" header. This forces the tool to use our modified header files. Then type:

    $ ~/clang-llvm/llvm-project/build/bin/micropy-convert FILENAME.cpp --
    
The converted output will be visible on the terminal, as well as an output.txt file located within the same folder.
