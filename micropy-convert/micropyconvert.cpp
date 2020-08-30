//------------------------------------------------------------------------------
// Micropython-Convert Demonstrates:
//
// * How to convert Arduino Sketches to Micropython
// * How to use AST matchers to find interesting AST nodes.
// * How to use the Rewriter API to rewrite the source code.
//
// Ashutosh Pandey (ashutoshpandey123456@gmail.com)
// This code is in the public domain
//------------------------------------------------------------------------------
#include <string>

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/AST/Expr.h"

using namespace std;
using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;

static llvm::cl::OptionCategory MatcherSampleCategory("Matcher Sample");

//IfStatementHandler Class: All Rewriting For IF statements done here.

class IfStmtHandler : public MatchFinder::MatchCallback {
public:
  IfStmtHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

  virtual void run(const MatchFinder::MatchResult &Result) {
    // The matched 'if' statement was bound to 'ifStmt'.
    if (const IfStmt *IfS = Result.Nodes.getNodeAs<clang::IfStmt>("ifStmt")) {
      const Stmt *Then = IfS->getThen();
      Rewrite.InsertText(Then->getBeginLoc(), "#if part\n", true, true);

      if (const Stmt *Else = IfS->getElse()) {
        Rewrite.InsertText(Else->getBeginLoc(), "#else part\n", true, true);
      }
    }
  }

private:
  Rewriter &Rewrite;
};

//ForLoopHandler Class: All Rewriting For For Loop statements done here.

class IncrementForLoopHandler : public MatchFinder::MatchCallback {
public:
  IncrementForLoopHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

  virtual void run(const MatchFinder::MatchResult &Result) {
    const VarDecl *IncVar = Result.Nodes.getNodeAs<VarDecl>("incVarName");
    Rewrite.InsertText(IncVar->getBeginLoc(), "#incvar/n", true, true);
  }

private:
  Rewriter &Rewrite;
};

//PinMode Class: All Rewriting For PinMode statements done here.

class pinModeVariableHandler : public MatchFinder::MatchCallback {
public:
   pinModeVariableHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::CallExpr* pm = Results.Nodes.getNodeAs<clang::CallExpr>("pinMode");
    Rewrite.ReplaceText(pm->getBeginLoc(), "Pin.mode");
    Rewrite.InsertText(pm->getBeginLoc(), "#from machine import pin at start of code\n", true, true);

  }

private:
  Rewriter &Rewrite;
};

//Handler for Void Loop() Class: All Rewriting For void loop statements done here. Void loop() is rewritten as While True:

class loopExprHandler : public MatchFinder::MatchCallback {
public:
   loopExprHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::FunctionDecl* loop = Results.Nodes.getNodeAs<clang::FunctionDecl>("loopexpr");
    Rewrite.RemoveText(loop->getLocation()); 
    Rewrite.ReplaceText(loop->getBeginLoc(), "While True:");
    Rewrite.ReplaceText(loop->getLocation(), " ");
  }

private:
  Rewriter &Rewrite;
};

//Handler for delay() function: delay() is rewritten as time.sleep_ms

class delayHandler : public MatchFinder::MatchCallback {
public:
   delayHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::CallExpr* delayfinder = Results.Nodes.getNodeAs<clang::CallExpr>("delay");
    Rewrite.ReplaceText(delayfinder->getBeginLoc(), "utime.sleep_ms");

  }

private:
  Rewriter &Rewrite;
};

//Handler for Void Setup() Class: Void Setup is Deleted as It does not occur in Micropython Statements

class setupHandler : public MatchFinder::MatchCallback {
public:
   setupHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::FunctionDecl* setupfinder = Results.Nodes.getNodeAs<clang::FunctionDecl>("setupfunc"); 
    Rewrite.RemoveText(setupfinder->getLocation());
    Rewrite.RemoveText(setupfinder->getBeginLoc());
    Rewrite.ReplaceText(setupfinder->getBeginLoc(), " ");
  }

private:
  Rewriter &Rewrite;
};


//Handler for CompoundStatements: Curly Braces are not required in Micropython and Can be removed. Since  project with improper indentation might become hard to understand
// it will insert a # (comment statement) before each {}

class compoundStmtHandler : public MatchFinder::MatchCallback {
public:
   compoundStmtHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::Stmt* compoundstmtfinder = Results.Nodes.getNodeAs<clang::Stmt>("compoundstmt");
    Rewrite.InsertText(compoundstmtfinder->getBeginLoc(), "#", true, true);
    Rewrite.InsertText(compoundstmtfinder->getEndLoc(), "#", true, true);
  }

private:
  Rewriter &Rewrite;
};

//Handler for power expression. converts pow to math.pow

class powerHandler : public MatchFinder::MatchCallback {
public:
   powerHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::Stmt* powfinder = Results.Nodes.getNodeAs<clang::Stmt>("pow");
    Rewrite.InsertText(powfinder->getBeginLoc(), "math.", true, true);
  }

private:
  Rewriter &Rewrite;
};

//Handler for square root expression. converts sqrt to math.sqrt

class sqrtHandler : public MatchFinder::MatchCallback {
public:
   sqrtHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::Stmt* sqrtfinder = Results.Nodes.getNodeAs<clang::Stmt>("sqrt");
    Rewrite.InsertText(sqrtfinder->getBeginLoc(), "math.", true, true);
  }

private:
  Rewriter &Rewrite;
};

//Handler for sin expression. converts sin to math.sin

class sinHandler : public MatchFinder::MatchCallback {
public:
   sinHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::Stmt* sinfinder = Results.Nodes.getNodeAs<clang::Stmt>("sin");
    Rewrite.InsertText(sinfinder->getBeginLoc(), "math.", true, true);
  }

private:
  Rewriter &Rewrite;
};

//Handler for cos expression. converts cos to math.cos

class cosHandler : public MatchFinder::MatchCallback {
public:
   cosHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::Stmt* cosfinder = Results.Nodes.getNodeAs<clang::Stmt>("cos");
    Rewrite.InsertText(cosfinder->getBeginLoc(), "math.", true, true);
  }

private:
  Rewriter &Rewrite;
};

//Handler for tan expression. converts tan to math.tan

class tanHandler : public MatchFinder::MatchCallback {
public:
   tanHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::Stmt* tanfinder = Results.Nodes.getNodeAs<clang::Stmt>("tan");
    Rewrite.InsertText(tanfinder->getBeginLoc(), "math.", true, true);
  }

private:
  Rewriter &Rewrite;
};

//Handler for delay() function: delay() is rewritten as time.sleep_ms

class delayMicrosecondsHandler : public MatchFinder::MatchCallback {
public:
   delayMicrosecondsHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::CallExpr* delayMicrosecondsfinder = Results.Nodes.getNodeAs<clang::CallExpr>("delayMicroseconds");
    Rewrite.ReplaceText(delayMicrosecondsfinder->getBeginLoc(), "utime.sleep_us");
  }

private:
  Rewriter &Rewrite;
};

//Handler for delay() function: delay() is rewritten as time.sleep_ms

class millisHandler : public MatchFinder::MatchCallback {
public:
   millisHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::CallExpr* millisfinder = Results.Nodes.getNodeAs<clang::CallExpr>("millis");
    Rewrite.ReplaceText(millisfinder->getBeginLoc(), "utime.ticks_ms");
  }

private:
  Rewriter &Rewrite;
};

//Handler for delay() function: delay() is rewritten as time.sleep_ms

class microsHandler : public MatchFinder::MatchCallback {
public:
   microsHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::CallExpr* microsfinder = Results.Nodes.getNodeAs<clang::CallExpr>("micros");
    Rewrite.ReplaceText(microsfinder->getBeginLoc(), "utime.ticks_us");
  }

private:
  Rewriter &Rewrite;
};

//Handler for delay() function: delay() is rewritten as time.sleep_ms

class pulseInHandler : public MatchFinder::MatchCallback {
public:
   pulseInHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::CallExpr* pulseInfinder = Results.Nodes.getNodeAs<clang::CallExpr>("pulseIn");
    Rewrite.ReplaceText(pulseInfinder->getBeginLoc(), "machine.time_pulse_us");
  }

private:
  Rewriter &Rewrite;
};

//Handler for PinMode Pin. converts pin number  to p<pinNumber>

class pinModePinHandler : public MatchFinder::MatchCallback {
public:
   pinModePinHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::Stmt* pinModePinfinder = Results.Nodes.getNodeAs<clang::Stmt>("pinModePin");
    Rewrite.InsertText(pinModePinfinder->getBeginLoc(), "p", true, true);
  }

private:
  Rewriter &Rewrite;
};
//Handler for INPUT keyword converts to IN

class inputHandler : public MatchFinder::MatchCallback {
public:
   inputHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::Stmt* inputfinder = Results.Nodes.getNodeAs<clang::Stmt>("INPUT");
        Rewrite.ReplaceText(inputfinder->getBeginLoc(), "IN");
  }

private:
  Rewriter &Rewrite;
};
 //Handler for OUTPUT keyword converts to OUT

class outputHandler : public MatchFinder::MatchCallback {
public:
   outputHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::Stmt* outputfinder = Results.Nodes.getNodeAs<clang::Stmt>("OUTPUT");
    Rewrite.ReplaceText(outputfinder->getBeginLoc(), "OUT");

  }

private:
  Rewriter &Rewrite;
};
//Handler for INPUT_PULLUP keyword converts to PULL_UP

class inputpullupHandler : public MatchFinder::MatchCallback {
public:
   inputpullupHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::Stmt* inputpullupfinder = Results.Nodes.getNodeAs<clang::Stmt>("INPUT_PULLUP");
    Rewrite.ReplaceText(inputpullupfinder->getBeginLoc(), "PULL_UP");
  }

private:
  Rewriter &Rewrite;
 
};

//Handler for isAlpha function: rewritten as ure.match()

class isAlphaHandler : public MatchFinder::MatchCallback {
public:
   isAlphaHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::CallExpr* isAlphafinder = Results.Nodes.getNodeAs<clang::CallExpr>("isAlpha");
    Rewrite.ReplaceText(isAlphafinder->getBeginLoc(), "ure.match");
    Rewrite.InsertText(isAlphafinder->getBeginLoc(), "#import ure at start of code\n", true, true);

  }

private:
  Rewriter &Rewrite;
};

//Handler for variable in isAlpha function: regex is inserted

class isAlphaVarHandler : public MatchFinder::MatchCallback {
public:
   isAlphaVarHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::DeclRefExpr* isAlphaVarfinder = Results.Nodes.getNodeAs<clang::DeclRefExpr>("isAlphaVar");
    Rewrite.InsertText(isAlphaVarfinder->getBeginLoc(), "'[A-Za-z]', ");
  }

private:
  Rewriter &Rewrite;
};

//Handler for isAlphaNumeric function: rewritten as ure.match()

class isAlphaNumericHandler : public MatchFinder::MatchCallback {
public:
   isAlphaNumericHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::CallExpr* isAlphaNumericfinder = Results.Nodes.getNodeAs<clang::CallExpr>("isAlphaNumeric");
    Rewrite.ReplaceText(isAlphaNumericfinder->getBeginLoc(), "ure.match");
    Rewrite.InsertText(isAlphaNumericfinder->getBeginLoc(), "#import ure at start of code\n", true, true);

  }

private:
  Rewriter &Rewrite;
};

//Handler for variable in isAlphaNumeric function: regex is inserted

class isAlphaNumericVarHandler : public MatchFinder::MatchCallback {
public:
   isAlphaNumericVarHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::DeclRefExpr* isAlphaNumericVarfinder = Results.Nodes.getNodeAs<clang::DeclRefExpr>("isAlphaNumericVar");
    Rewrite.InsertText(isAlphaNumericVarfinder->getBeginLoc(), "'[A-Za-z0-9]', ");
  }

private:
  Rewriter &Rewrite;
};

//Handler for isAscii function: isAscii is rewritten as ure.match()

class isAsciiHandler : public MatchFinder::MatchCallback {
public:
   isAsciiHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::CallExpr* isAsciifinder = Results.Nodes.getNodeAs<clang::CallExpr>("isAscii");
    Rewrite.ReplaceText(isAsciifinder->getBeginLoc(), "ure.match");
    Rewrite.InsertText(isAsciifinder->getBeginLoc(), "#import ure at start of code\n", true, true);

  }

private:
  Rewriter &Rewrite;
};

//Handler for variable in isAscii function: regex is inserted.

class isAsciiVarHandler : public MatchFinder::MatchCallback {
public:
   isAsciiVarHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::DeclRefExpr* isAsciiVarfinder = Results.Nodes.getNodeAs<clang::DeclRefExpr>("isAsciiVar");
    Rewrite.InsertText(isAsciiVarfinder->getBeginLoc(), "'\\w\\W' ");
  }

private:
  Rewriter &Rewrite;
};

//Handler for isDigit function: isDigit is rewritten as ure.match()

class isDigitHandler : public MatchFinder::MatchCallback {
public:
   isDigitHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::CallExpr* isDigitfinder = Results.Nodes.getNodeAs<clang::CallExpr>("isDigit");
    Rewrite.ReplaceText(isDigitfinder->getBeginLoc(), "ure.match");
    Rewrite.InsertText(isDigitfinder->getBeginLoc(), "#import ure at start of code\n", true, true);

  }

private:
  Rewriter &Rewrite;
};

//Handler for variable in isDigit function: regex is inserted.

class isDigitVarHandler : public MatchFinder::MatchCallback {
public:
   isDigitVarHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::DeclRefExpr* isDigitVarfinder = Results.Nodes.getNodeAs<clang::DeclRefExpr>("isDigitVar");
    Rewrite.InsertText(isDigitVarfinder->getBeginLoc(), "'\\d' ");
  }

private:
  Rewriter &Rewrite;
};

//Handler for isLowerCase function: isLowerCase is rewritten as ure.match()

class isLowerCaseHandler : public MatchFinder::MatchCallback {
public:
   isLowerCaseHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::CallExpr* isLowerCasefinder = Results.Nodes.getNodeAs<clang::CallExpr>("isLowerCase");
    Rewrite.ReplaceText(isLowerCasefinder->getBeginLoc(), "ure.match");
    Rewrite.InsertText(isLowerCasefinder->getBeginLoc(), "#import ure at start of code\n", true, true);

  }

private:
  Rewriter &Rewrite;
};

//Handler for variable in isLowerCase function: regex is inserted.

class isLowerCaseVarHandler : public MatchFinder::MatchCallback {
public:
   isLowerCaseVarHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::DeclRefExpr* isLowerCaseVarfinder = Results.Nodes.getNodeAs<clang::DeclRefExpr>("isLowerCaseVar");
    Rewrite.InsertText(isLowerCaseVarfinder->getBeginLoc(), "'[a-z]', ");
  }

private:
  Rewriter &Rewrite;
};

//Handler for isPunct function: isPunct is rewritten as ure.match

class isPunctHandler : public MatchFinder::MatchCallback {
public:
   isPunctHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::CallExpr* isPunctfinder = Results.Nodes.getNodeAs<clang::CallExpr>("isPunct");
    Rewrite.ReplaceText(isPunctfinder->getBeginLoc(), "ure.match");
    Rewrite.InsertText(isPunctfinder->getBeginLoc(), "#import ure at start of code\n", true, true);

  }

private:
  Rewriter &Rewrite;
};

//Handler for variable in isPunct function: regex is inserted.

class isPunctVarHandler : public MatchFinder::MatchCallback {
public:
   isPunctVarHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::DeclRefExpr* isPunctVarfinder = Results.Nodes.getNodeAs<clang::DeclRefExpr>("isPunctVar");
    Rewrite.InsertText(isPunctVarfinder->getBeginLoc(), "'\\W' ");
  }

private:
  Rewriter &Rewrite;
};

//Handler for isSpace function: isSpace is rewritten as ure.match

class isSpaceHandler : public MatchFinder::MatchCallback {
public:
   isSpaceHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::CallExpr* isSpacefinder = Results.Nodes.getNodeAs<clang::CallExpr>("isSpace");
    Rewrite.ReplaceText(isSpacefinder->getBeginLoc(), "ure.match");
    Rewrite.InsertText(isSpacefinder->getBeginLoc(), "#import ure at start of code\n", true, true);

  }

private:
  Rewriter &Rewrite;
};

//Handler for variable in isSpace function: regex is inserted.

class isSpaceVarHandler : public MatchFinder::MatchCallback {
public:
   isSpaceVarHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::DeclRefExpr* isSpaceVarfinder = Results.Nodes.getNodeAs<clang::DeclRefExpr>("isSpaceVar");
    Rewrite.InsertText(isSpaceVarfinder->getBeginLoc(), "'\\f\\n\\r\\t\\v\\s', ");
  }

private:
  Rewriter &Rewrite;
};

//Handler for isUpperCase function: isUpperCase is rewritten as ure.match

class isUpperCaseHandler : public MatchFinder::MatchCallback {
public:
   isUpperCaseHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::CallExpr* isUpperCasefinder = Results.Nodes.getNodeAs<clang::CallExpr>("isUpperCase");
    Rewrite.ReplaceText(isUpperCasefinder->getBeginLoc(), "ure.match");
    Rewrite.InsertText(isUpperCasefinder->getBeginLoc(), "#import ure at start of code\n", true, true);

  }

private:
  Rewriter &Rewrite;
};

//Handler for variable in isUpperCase function: regex is inserted.

class isUpperCaseVarHandler : public MatchFinder::MatchCallback {
public:
   isUpperCaseVarHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::DeclRefExpr* isUpperCaseVarfinder = Results.Nodes.getNodeAs<clang::DeclRefExpr>("isUpperCaseVar");
    Rewrite.InsertText(isUpperCaseVarfinder->getBeginLoc(), "'[A-Z]', ");
  }

private:
  Rewriter &Rewrite;
};

//Handler for isWhitespace function: isWhitespace is rewritten as ure.match

class isWhitespaceHandler : public MatchFinder::MatchCallback {
public:
   isWhitespaceHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::CallExpr* isWhitespacefinder = Results.Nodes.getNodeAs<clang::CallExpr>("isWhitespace");
    Rewrite.ReplaceText(isWhitespacefinder->getBeginLoc(), "ure.match");
    Rewrite.InsertText(isWhitespacefinder->getBeginLoc(), "#import ure at start of code\n", true, true);

  }

private:
  Rewriter &Rewrite;
};

//Handler for variable in isWhitespace function: regex is inserted.

class isWhitespaceVarHandler : public MatchFinder::MatchCallback {
public:
   isWhitespaceVarHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::DeclRefExpr* isWhitespaceVarfinder = Results.Nodes.getNodeAs<clang::DeclRefExpr>("isWhitespaceVar");
    Rewrite.InsertText(isWhitespaceVarfinder->getBeginLoc(), "'\\s\\t', ");
  }

private:
  Rewriter &Rewrite;
};

//Handler for analogRead function: analogRead is converted to ADC.read_u16

class analogReadHandler : public MatchFinder::MatchCallback {
public:
   analogReadHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::CallExpr* analogReadfinder = Results.Nodes.getNodeAs<clang::CallExpr>("analogRead");
    Rewrite.ReplaceText(analogReadfinder->getBeginLoc(), "ADC.read_u16");
    Rewrite.InsertText(analogReadfinder->getBeginLoc(), "#import machine at start of code\n", true, true);

  }

private:
  Rewriter &Rewrite;
};

//Handler for analogRead function: analogRead is converted to machine.PWM

class analogWriteHandler : public MatchFinder::MatchCallback {
public:
   analogWriteHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::CallExpr* analogWritefinder = Results.Nodes.getNodeAs<clang::CallExpr>("analogWrite");
    Rewrite.ReplaceText(analogWritefinder->getBeginLoc(), "machine.PWM");
    Rewrite.InsertText(analogWritefinder->getBeginLoc(), "#import machine at start of code\n", true, true);

  }

private:
  Rewriter &Rewrite;
};

//Handler for digitalRead function: digitalRead is converted to Pin.value. Whether it is read or write is determined by the number of Arguments

class digitalReadHandler : public MatchFinder::MatchCallback {
public:
   digitalReadHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::CallExpr* digitalReadfinder = Results.Nodes.getNodeAs<clang::CallExpr>("digitalRead");
    Rewrite.ReplaceText(digitalReadfinder->getBeginLoc(), "Pin.value");
  }

private:
  Rewriter &Rewrite;
};

//Handler for digitalWrite function: digitalWrite is converted to Pin.value. Whether it is read or write is determined by the number of Arguments.

class digitalWriteHandler : public MatchFinder::MatchCallback {
public:
   digitalWriteHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::CallExpr* digitalWritefinder = Results.Nodes.getNodeAs<clang::CallExpr>("digitalWrite");
    Rewrite.ReplaceText(digitalWritefinder->getBeginLoc(), "Pin.value");
  }

private:
  Rewriter &Rewrite;
};

//Handler for the constant Pi. Pi is converted to math.pi

class piHandler : public MatchFinder::MatchCallback {
public:
   piHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::DeclRefExpr* pifinder = Results.Nodes.getNodeAs<clang::DeclRefExpr>("PI");
    Rewrite.ReplaceText(pifinder->getBeginLoc(), "math.pi");
  }

private:
  Rewriter &Rewrite;
};

//Handler for the constant e. e is converted to math.e

class eulerHandler : public MatchFinder::MatchCallback {
public:
   eulerHandler(Rewriter &Rewrite) : Rewrite(Rewrite)  {}

virtual void run(const MatchFinder::MatchResult &Results) {
    const clang::DeclRefExpr* eulerfinder = Results.Nodes.getNodeAs<clang::DeclRefExpr>("EULER");
    Rewrite.ReplaceText(eulerfinder->getBeginLoc(), "math.e");
  }

private:
  Rewriter &Rewrite;
};

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser. It registers a couple of matchers and runs them on
// the AST.
class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(Rewriter &R) : HandlerForIf(R), HandlerForFor(R), HandlerForpinMode(R), HandlerForLoopExpr(R), HandlerForDelay(R), HandlerForSetup(R), HandlerForCompoundStmt(R), 
  HandlerForPower(R), HandlerForSqrt(R), HandlerForSin(R), HandlerForCos(R), HandlerForTan(R), HandlerForDelayMicroseconds(R), HandlerForMillis(R), HandlerForMicros(R), HandlerForPulseIn(R),
  HandlerForPinModePin(R), HandlerForINPUT(R), HandlerForOUTPUT(R), HandlerForINPUTPULLUP(R), HandlerForIsAlpha(R),HandlerForIsAlphaVar(R), HandlerForIsAlphaNumeric(R), 
  HandlerForIsAlphaNumericVar(R), HandlerForIsAscii(R), HandlerForIsAsciiVar(R), HandlerForIsDigit(R), HandlerForIsDigitVar(R), HandlerForIsLowerCase(R), HandlerForIsLowerCaseVar(R),
   HandlerForIsPunct(R), HandlerForIsPunctVar(R), HandlerForIsSpace(R), HandlerForIsSpaceVar(R), HandlerForIsUpperCase(R), HandlerForIsUpperCaseVar(R), HandlerForIsWhitespace(R), HandlerForIsWhitespaceVar(R),
   HandlerForAnalogRead(R), HandlerForAnalogWrite(R), HandlerForDigitalRead(R), HandlerForDigitalWrite(R), HandlerForPi(R), HandlerForEuler(R){
    // Add a simple matcher for finding 'if' statements.
    Matcher.addMatcher(ifStmt().bind("ifStmt"), &HandlerForIf);

    // Add a complex matcher for finding 'for' loops with an initializer set
    // to 0, < comparison in the codition and an increment. For example:
    //
    //  for (int i = 0; i < N; ++i). Just to test it out. Will change for to for in range
    Matcher.addMatcher(
        forStmt(hasLoopInit(declStmt(hasSingleDecl(
                    varDecl(hasInitializer(integerLiteral(equals(0))))
                        .bind("initVarName")))),
                hasIncrement(unaryOperator(
                    hasOperatorName("++"),
                    hasUnaryOperand(declRefExpr(to(
                        varDecl(hasType(isInteger())).bind("incVarName")))))),
                hasCondition(binaryOperator(
                    hasOperatorName("<"),
                    hasLHS(ignoringParenImpCasts(declRefExpr(to(
                        varDecl(hasType(isInteger())).bind("condVarName"))))),
                    hasRHS(expr(hasType(isInteger()))))))
            .bind("forLoop"),
        &HandlerForFor);

//Add A matcher for PinMode

Matcher.addMatcher(
	callExpr(isExpansionInMainFile(), callee(functionDecl(hasName("pinMode")))).bind("pinMode"), &HandlerForpinMode);

//Add A matcher for void_loop function of Arduino

Matcher.addMatcher(
  functionDecl(isExpansionInMainFile(), hasName("loop"), parameterCountIs(0)).bind("loopexpr"), &HandlerForLoopExpr); 

 //Add A matcher for delay() function
Matcher.addMatcher(
  callExpr(isExpansionInMainFile(), callee(functionDecl(hasName("delay")))).bind("delay"), &HandlerForDelay);

 //Add A matcher to delete Void Setup() 
 Matcher.addMatcher(
   functionDecl(isExpansionInMainFile(), hasName("setup")).bind("setupfunc"), &HandlerForSetup); 

//Add A matcher to remove { } braces
Matcher.addMatcher(
  stmt(isExpansionInMainFile(), compoundStmt()).bind("compoundstmt"), &HandlerForCompoundStmt);

//Add a matcher to convert power to math.pow
Matcher.addMatcher(
  stmt(isExpansionInMainFile(), has(declRefExpr(throughUsingDecl(hasName("pow"))))).bind("pow"), &HandlerForPower);

//Add a matcher to convert sqrt to math.sqrt
Matcher.addMatcher(
  stmt(isExpansionInMainFile(), has(declRefExpr(throughUsingDecl(hasName("sqrt"))))).bind("sqrt"), &HandlerForSqrt);

//Add a matcher to convert sin to math.sin
Matcher.addMatcher(
  stmt(isExpansionInMainFile(), has(declRefExpr(throughUsingDecl(hasName("sin"))))).bind("sin"), &HandlerForSin);

//Add a matcher to convert cos to math.cos
Matcher.addMatcher(
  stmt(isExpansionInMainFile(), has(declRefExpr(throughUsingDecl(hasName("cos"))))).bind("cos"), &HandlerForCos);

//Add a matcher to convert tan to math.tan
Matcher.addMatcher(
  stmt(isExpansionInMainFile(), has(declRefExpr(throughUsingDecl(hasName("tan"))))).bind("tan"), &HandlerForTan);

//Add a matcher to convert delayMicroseconds() to its Micropython equivalent.
Matcher.addMatcher(
  callExpr(isExpansionInMainFile(), callee(functionDecl(hasName("delayMicroseconds")))).bind("delayMicroseconds"), &HandlerForDelayMicroseconds);

//Add a matcher to convert millis() to its Micropython equivalent.
Matcher.addMatcher(
  callExpr(isExpansionInMainFile(), callee(functionDecl(hasName("millis")))).bind("millis"), &HandlerForMillis);

//Add a matcher to convert micros() to its Micropython equivalent.
Matcher.addMatcher(
  callExpr(isExpansionInMainFile(), callee(functionDecl(hasName("micros")))).bind("micros"), &HandlerForMicros);

  //Add a matcher to convert micros() to its Micropython equivalent.
Matcher.addMatcher(
  callExpr(isExpansionInMainFile(), callee(functionDecl(hasName("pulseIn")))).bind("pulseIn"), &HandlerForPulseIn);

  //Add a matcher to convert pin numbers to pin numbers with prefix 'p' inside Pin.Mode.
Matcher.addMatcher(
  stmt(isExpansionInMainFile(), hasAncestor(callExpr(callee(functionDecl(hasName("pinMode"))))), has(integerLiteral())).bind("pinModePin"), &HandlerForPinModePin);

    //Add a matcher to convert INPUT to IN. pinmode uses Pin.Mode(PIN.IN)
Matcher.addMatcher(
  stmt(isExpansionInMainFile(), declRefExpr(to(varDecl(hasName("INPUT"))))).bind("INPUT"), &HandlerForINPUT);

//Add a matcher to convert Output to OUT. pinmode uses Pin.Mode(PIN.OUT)
Matcher.addMatcher(
  stmt(isExpansionInMainFile(), declRefExpr(to(varDecl(hasName("OUTPUT"))))).bind("OUTPUT"), &HandlerForOUTPUT);

//Add a matcher to convert INPUT_PULLUP to PULLUP pinmode uses Pin.Mode(PIN.PULL_UP)
Matcher.addMatcher(
  stmt(isExpansionInMainFile(), declRefExpr(to(varDecl(hasName("INPUT_PULLUP"))))).bind("INPUT_PULLUP"), &HandlerForINPUTPULLUP);

//Add a matcher to convert isAlpha to ure.match()
Matcher.addMatcher(
  callExpr(isExpansionInMainFile(), callee(functionDecl(hasName("isAlpha")))).bind("isAlpha"), &HandlerForIsAlpha);

//Add a matcher to add the regex string inside the isAlpha()
Matcher.addMatcher(
  declRefExpr(isExpansionInMainFile(), to(varDecl()), hasAncestor(callExpr(callee(functionDecl(hasName("isAlpha")))))).bind("isAlphaVar"), &HandlerForIsAlphaVar);

//Add a matcher to convert isAlphaNumeric to ure.match()
Matcher.addMatcher(
  callExpr(isExpansionInMainFile(), callee(functionDecl(hasName("isAlphaNumeric")))).bind("isAlphaNumeric"), &HandlerForIsAlphaNumeric);

//Add a matcher to add the regex string inside the isAlphaNumeric()
Matcher.addMatcher(
  declRefExpr(isExpansionInMainFile(), to(varDecl()), hasAncestor(callExpr(callee(functionDecl(hasName("isAlphaNumeric")))))).bind("isAlphaNumericVar"), &HandlerForIsAlphaNumericVar);

//Add a matcher to convert isAscii to ure.match()
Matcher.addMatcher(
  callExpr(isExpansionInMainFile(), callee(functionDecl(hasName("isAscii")))).bind("isAscii"), &HandlerForIsAscii);

//Add a matcher to add the regex string inside the isAscii()
Matcher.addMatcher(
  declRefExpr(isExpansionInMainFile(), to(varDecl()), hasAncestor(callExpr(callee(functionDecl(hasName("isAscii")))))).bind("isAsciiVar"), &HandlerForIsAsciiVar);

//Add a matcher to convert isDigit to ure.match()
Matcher.addMatcher(
  callExpr(isExpansionInMainFile(), callee(functionDecl(hasName("isDigit")))).bind("isDigit"), &HandlerForIsDigit);

//Add a matcher to add the regex string inside the isDigit()
Matcher.addMatcher(
  declRefExpr(isExpansionInMainFile(), to(varDecl()), hasAncestor(callExpr(callee(functionDecl(hasName("isDigit")))))).bind("isDigitVar"), &HandlerForIsDigitVar);

//Add a matcher to convert isLowerCase to ure.match()
Matcher.addMatcher(
  callExpr(isExpansionInMainFile(), callee(functionDecl(hasName("isLowerCase")))).bind("isLowerCase"), &HandlerForIsLowerCase);

//Add a matcher to add the regex string inside the isLowerCase()
Matcher.addMatcher(
  declRefExpr(isExpansionInMainFile(), to(varDecl()), hasAncestor(callExpr(callee(functionDecl(hasName("isLowerCase")))))).bind("isLowerCaseVar"), &HandlerForIsLowerCaseVar);

//Add a matcher to convert isPunct to ure.match()
Matcher.addMatcher(
  callExpr(isExpansionInMainFile(), callee(functionDecl(hasName("isPunct")))).bind("isPunct"), &HandlerForIsPunct);

//Add a matcher to add the regex string inside the isPunct()
Matcher.addMatcher(
  declRefExpr(isExpansionInMainFile(), to(varDecl()), hasAncestor(callExpr(callee(functionDecl(hasName("isPunct")))))).bind("isPunctVar"), &HandlerForIsPunctVar);

//Add a matcher to convert isSpace to ure.match()
Matcher.addMatcher(
  callExpr(isExpansionInMainFile(), callee(functionDecl(hasName("isSpace")))).bind("isSpace"), &HandlerForIsSpace);

//Add a matcher to add the regex string inside the isSpace()
Matcher.addMatcher(
  declRefExpr(isExpansionInMainFile(), to(varDecl()), hasAncestor(callExpr(callee(functionDecl(hasName("isSpace")))))).bind("isSpaceVar"), &HandlerForIsSpaceVar);

//Add a matcher to convert isUpperCase to ure.match()
Matcher.addMatcher(
  callExpr(isExpansionInMainFile(), callee(functionDecl(hasName("isUpperCase")))).bind("isUpperCase"), &HandlerForIsUpperCase);

//Add a matcher to add the regex string inside the isUpperCase()
Matcher.addMatcher(
  declRefExpr(isExpansionInMainFile(), to(varDecl()), hasAncestor(callExpr(callee(functionDecl(hasName("isUpperCase")))))).bind("isUpperCaseVar"), &HandlerForIsUpperCaseVar);

//Add a matcher to convert isWhitespace to ure.match()
Matcher.addMatcher(
  callExpr(isExpansionInMainFile(), callee(functionDecl(hasName("isWhitespace")))).bind("isWhitespace"), &HandlerForIsWhitespace);

//Add a matcher to add the regex string inside the isWhitespace()
Matcher.addMatcher(
  declRefExpr(isExpansionInMainFile(), to(varDecl()), hasAncestor(callExpr(callee(functionDecl(hasName("isWhitespace")))))).bind("isWhitespaceVar"), &HandlerForIsWhitespaceVar);
//Add a matcher to convert analogRead to its micropython equivalent.
Matcher.addMatcher(
  callExpr(isExpansionInMainFile(), callee(functionDecl(hasName("analogRead")))).bind("analogRead"), &HandlerForAnalogRead);

//Add a matcher to convert analogWrite to its micropython equivalent.
Matcher.addMatcher(
  callExpr(isExpansionInMainFile(), callee(functionDecl(hasName("analogWrite")))).bind("analogWrite"), &HandlerForAnalogWrite);

//Add a matcher to convert digitalRead to its micropython equivalent.
Matcher.addMatcher(
  callExpr(isExpansionInMainFile(), callee(functionDecl(hasName("digitalRead")))).bind("digitalRead"), &HandlerForDigitalRead);

//Add a matcher to convert digitalWrite to its micropython equivalent.
Matcher.addMatcher(
  callExpr(isExpansionInMainFile(), callee(functionDecl(hasName("digitalWrite")))).bind("digitalWrite"), &HandlerForDigitalWrite);

//Add a matcher to convert the constant Pi to its micropython equivalent.
Matcher.addMatcher(
  declRefExpr(isExpansionInMainFile(), to(varDecl(hasName("PI")))).bind("PI"), &HandlerForPi);

//Add a matcher to convert the constant e to its micropython equivalent.
Matcher.addMatcher(
  declRefExpr(isExpansionInMainFile(), to(varDecl(hasName("EULER")))).bind("EULER"), &HandlerForEuler);

  }

  void HandleTranslationUnit(ASTContext &Context) override {
    // Run the matchers when we have the whole TU parsed.
    Matcher.matchAST(Context);

  }

private:
  IfStmtHandler HandlerForIf;
  IncrementForLoopHandler HandlerForFor;
  pinModeVariableHandler HandlerForpinMode;
  loopExprHandler HandlerForLoopExpr;
  delayHandler HandlerForDelay;
  setupHandler HandlerForSetup;
  compoundStmtHandler HandlerForCompoundStmt;
  powerHandler HandlerForPower;
  sqrtHandler HandlerForSqrt;
  sinHandler HandlerForSin;
  cosHandler HandlerForCos;
  tanHandler HandlerForTan;
  delayMicrosecondsHandler HandlerForDelayMicroseconds;
  millisHandler HandlerForMillis;
  microsHandler HandlerForMicros;
  pulseInHandler HandlerForPulseIn;
  pinModePinHandler HandlerForPinModePin;
  inputHandler HandlerForINPUT;
  outputHandler HandlerForOUTPUT;
  inputpullupHandler HandlerForINPUTPULLUP;
  isAlphaHandler HandlerForIsAlpha;
  isAlphaVarHandler HandlerForIsAlphaVar;
  isAlphaNumericHandler HandlerForIsAlphaNumeric;
  isAlphaNumericVarHandler HandlerForIsAlphaNumericVar;
  isAsciiHandler HandlerForIsAscii;
  isAsciiVarHandler HandlerForIsAsciiVar;
  isDigitHandler HandlerForIsDigit;
  isDigitVarHandler HandlerForIsDigitVar;
  isLowerCaseHandler HandlerForIsLowerCase;
  isLowerCaseVarHandler HandlerForIsLowerCaseVar;
  isPunctHandler HandlerForIsPunct;
  isPunctVarHandler HandlerForIsPunctVar;
  isSpaceHandler HandlerForIsSpace;
  isSpaceVarHandler HandlerForIsSpaceVar;
  isUpperCaseHandler HandlerForIsUpperCase;
  isUpperCaseVarHandler HandlerForIsUpperCaseVar;
  isWhitespaceHandler HandlerForIsWhitespace;
  isWhitespaceVarHandler HandlerForIsWhitespaceVar;
  analogReadHandler HandlerForAnalogRead;
  analogWriteHandler HandlerForAnalogWrite;
  digitalReadHandler HandlerForDigitalRead;
  digitalWriteHandler HandlerForDigitalWrite;
  piHandler HandlerForPi;
  eulerHandler HandlerForEuler;


  MatchFinder Matcher;
};

// For each source file provided to the tool, a new FrontendAction is created.
class MyFrontendAction : public ASTFrontendAction {
public:
  MyFrontendAction() {}
  void EndSourceFileAction() override {
   SourceManager &SM = TheRewriter.getSourceMgr();
   llvm::errs() << "** EndSourceFileAction for: "
                 << SM.getFileEntryForID(SM.getMainFileID())->getName() << "\n";
//Now emit the Rewritten Buffer
    TheRewriter.getEditBuffer(TheRewriter.getSourceMgr().getMainFileID())
        .write(llvm::outs());

std::error_code error_code;
        llvm::raw_fd_ostream outFile("output.txt", error_code, llvm::sys::fs::F_None);
     TheRewriter.getEditBuffer(SM.getMainFileID()).write(outFile); // --> this will write the result>
    outFile.close();

  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override {
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return std::make_unique<MyASTConsumer>(TheRewriter);
  }

private:
  Rewriter TheRewriter;
};

int main(int argc, const char **argv) {
  CommonOptionsParser op(argc, argv, MatcherSampleCategory);
  ClangTool Tool(op.getCompilations(), op.getSourcePathList());

  return Tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}
