#include "LiquidCrystal.h"
#include "Keypad.h"
#include "math.h"

#define MAX_SIZE 256

#define PI 3.1415926535897932384626433832795
#define EULER 2.718281828459045235360287471352

LiquidCrystal lcd(12, 11, 10, 9, 8, 7);

const int ROWS = 4;
const int COLS = 4;

char numpad[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'.', '0', '#', 'D'}
};

byte rowPins[ROWS] = {5, 4, 3, 2};
byte colPins[COLS] = {A3, A2, A1, A0};
Keypad customKeypad = Keypad(makeKeymap(numpad), rowPins, colPins, ROWS, COLS);


// globals
char function[MAX_SIZE];
char strFunc[MAX_SIZE];
char rpn[MAX_SIZE];
double values[30] = {0};

char valChar = 'a';
double tempNum = 0;

bool decimal = false; 
bool building = false;

double decimalMultiplier = 0.1;
int valIndex = 0;
int strindex = 0;

const char operators[] = "+-*/^{[]@$&"; // { - sqrt, [ - log, ] - ln
const char other[] = "ez{([]@$&"; // e - e, z - pi
const char unary[] = "{[]@$&!"; // @ - sin, $ - cos, & - tan
const char keys[] = "ABCD";

unsigned long timeSinceInput = 0;


void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.print("Booting");
  delay(1000);
  lcd.clear();
}

void loop() {
  defineConsts();
  lcd.print("Enter Function:");
  lcd.setCursor(0, 1);
  getInputFunc();
  formatFunc();

  Serial.println(function);

  printResult();
  reset();
}
// plant pi and e, find a better way later
void defineConsts() {
  int eIndex = 'e' - 'a';
  values[eIndex] = EULER;

  int piIndex = 'z' - 'a';
  values[piIndex] = PI;
}
// print char on lcd
void printChar(char ch) {
  switch(ch) {
    case '*':
      lcd.print((char)B10100101);
      break;
    case '/':
      lcd.print((char)B11111101);
      break;
    case '{':
      lcd.print((char)B11101000);
      break;
    case 'z':
      lcd.print((char)B11110111);
      break;
    case '[':
      lcd.print("log");
      break;
    case ']':
      lcd.print("ln");
      break;
    case '@':
      lcd.print("sin");
      break;
    case '$':
      lcd.print("cos");
      break;
    case '&':
      lcd.print("tan");
      break;
    default:
      lcd.print(ch);
  }
}
// print final lcd screen
void printResult() {
  double result = toRPN();
  lcd.clear();
  printFunc(0);
  lcd.setCursor(0, 1);
  lcd.print(result);

  while (customKeypad.getKey() != '#') { // wait for # input
    delay(10);
  }
}
// print function
void printFunc(int row) {
  lcd.setCursor(0, row);
  int size = strFuncSize();
  if (size + 1 > 16) { // print last 16 if doesnt fit
    int diff = size - 16;
    for (int i = diff; i < size; i++) {
      printChar(strFunc[i]);
    }
  } else { // iterate from front if fits on screen
    for (int i = 0; i < size; i++) {
      printChar(strFunc[i]);
    }
  }
}
// reset static variables/lcd
void reset() {
  memset(function, '\0', sizeof(function));
  memset(values, '\0', sizeof(values));
  memset(rpn, '\0', sizeof(rpn));
  memset(strFunc, '\0', sizeof(strFunc));
  tempNum = 0;
  decimal = false;
  decimalMultiplier = 0.1;
  valChar = 'a';
  valIndex = 0;
  strindex = 0;
  building = false;
  unsigned long timeSinceInput = 0;
  lcd.clear();
  lcd.home();
}
// build function for eval
void getInputFunc() {
  int i = 0;
  while (true) {
    char ch = customKeypad.getKey();
    if (ch == NO_KEY) {
      delay(10);
      continue;
    } else {
      if (isKey(ch)) {
        while(true) {
          char ch2 = customKeypad.getKey();
          if (ch2 == NO_KEY) {
            continue;
          } else {
            ch = getOp(ch, ch2);
            break;
          }
        }
      }
    }
    // check if anything except del(%) or enter
    if (ch != '%' && ch != '#') {
      switch(ch) { // handle multi character ops
        case '[':
          strFunc[strindex++] = 'l';
          strFunc[strindex++] = 'o';
          strFunc[strindex++] = 'g';
          break;
        case ']':
          strFunc[strindex++] = 'l';
          strFunc[strindex++] = 'n';
          break;
        case '@':
          strFunc[strindex++] = 's';
          strFunc[strindex++] = 'i';
          strFunc[strindex++] = 'n';
          break;
        case '$':
          strFunc[strindex++] = 'c';
          strFunc[strindex++] = 'o';
          strFunc[strindex++] = 's';
          break;
        case '&':
          strFunc[strindex++] = 't';
          strFunc[strindex++] = 'a';
          strFunc[strindex++] = 'n';
          break;
        case '%':
        case '#': // throw del and enter
          break;
        default:
          strFunc[strindex++] = ch; // add ch to string func
      }
      printFunc(1); // print stringfunc

      // check if anything except number
      if (isOp(ch) || isEx(ch) || ch == ')' || ch == '!') {
        // end number if building one
        if (building) {
          values[valIndex++] = tempNum;
          function[i++] = getValChar(); // add prev num
        }
        endNum(); // stops num construction when needed
        function[i++] = ch; //add ch
      } else {
        addNum(ch); // build num inbetween other inputs
      }
      // delete
    } else if (ch == '%') {
      if (i > 0) {
        i--;
        function[i] = '\0';
        lcd.setCursor(i, 1);
        lcd.print(" ");
        lcd.setCursor(i, 1);
      }
      // enter
    } else if (ch == '#') {
      if (building) {
        values[valIndex++] = tempNum;
        function[i++] = getValChar(); // add prev num
        endNum();
      }
      // stops num construction when needed
      return;
    }
  }
}
// get operator from input
char getOp(char ch, char ch2) {
  switch(ch) {
    case 'A':
      switch(ch2) {
        case 'A':
          return '+';
        case 'B':
          return '-';
        case 'C':
          return '*';
        case 'D':
          return '/';
        case '.':
          return '(';
        case '#':
          return ')';
        case '2':
          return 'e';
        case '3':
          return 'z';
      }
    case 'B':
      switch(ch2) {
        case 'A':
          return '^';
        case 'B':
          return '{'; // sqrt
        case 'C':
          return '['; // log base 10
        case 'D':
          return ']'; // ln
        // case '.':
        //   return '%'; // delete
      }
    case 'C':
      switch(ch2) {
        case 'A':
          return '@'; // sin
        case 'B':
          return '$'; // cos
        case 'C':
          return '&'; // tan
        // case 'D':
          
      }
    case 'D':
      switch(ch2) {
        case 'A':
          return '!';
        // case 'B':
        //   return '"'; // inverse
        // case 'C':
          
        // case 'D':
          
      }
  } 
}
//checks implicit mult
void formatFunc() {
  for (int i = 1; function[i] != '\0'; i++) {
    if (isEx(function[i])) {
      //add mult symbol before (/)/e/pi/sqrt
      if (!isOp(function[i - 1]) && !(function[i - 1] == '(')) {
        //push rest of func forward
        for (int j = funcSize(); j > i; j--) {
          function[j] = function[j - 1];
        }
        //add *
        function[i] = '*';
      }
    }
  }
}
//returns size of function
int funcSize() {
  int i = 0;
  while(function[i] != '\0') {
    i++;
  }
  return i;
}
//returns size of string function
int strFuncSize() {
  int i = 0;
  while(strFunc[i] != '\0') {
    i++;
  }
  return i;
}
//update numebr as needed
void addNum(char ch) {
  building = true;
  //toggle building on
  if (ch == '.' && !decimal) {
    decimal = true;
  } else if (isdigit(ch)) {
    if (!decimal) {
      //add to end of .
      tempNum = tempNum * 10 + (ch - '0');
    } else {
      //add to end of int
      tempNum += (ch - '0') * decimalMultiplier;
      decimalMultiplier *= 0.1;
    }
  }
}
//end number
void endNum() {
  building = false;
  //toggle off
  tempNum = 0;
  decimal = false;
  decimalMultiplier = 0.1;
}
//check if op
bool isOp(char ch) {
    return strchr(operators, ch) != nullptr;
}
//check if other
bool isEx(char ch) {;
    return strchr(other, ch) != nullptr;
}
//check if A-D
bool isKey(char ch) {;
    return strchr(keys, ch) != nullptr;
}
//check if unary op
bool isUnary(char ch) {;
    return strchr(unary, ch) != nullptr;
}
//get next char in the 'bet
char getValChar() {
  if (valChar == 'e' || valChar == 'z') {
    valChar++;
  }
  return valChar++;
}
//return precesende
int precedence(char op) {
  switch(op) {
    case '+':
    case '-':
      return 1;
    case '*':
    case '/':
      return 2;
    case '^':
    case '{':
    case '[':
    case ']':
    case '@':
    case '$':
    case '&':
      return 3;
    case '!':
      return 4;
    case '(':
      return 10;
  }
}
//infix to postfix
double toRPN() {
  char stack[MAX_SIZE];
  int stackTop = -1;
  int index = 0;

  for (int i = 0; function[i] != '\0'; i++) {
    Serial.println(rpn);
    char ch = function[i];

    if (ch >= 'a' && ch <= 'z') { // check if num
      rpn[index++] = ch;
    } else if (isUnary(ch)) { // check if unary op
      Serial.print("flag3 Adding to rpn : ");
      Serial.println(ch);
      stack[++stackTop] = ch;
    } else if (ch == '(') { // L parenthesis
      stack[++stackTop] = ch;
    } else if (ch == ')') { // R parenthesis
      while (stackTop >= 0 && stack[stackTop] != '(') {
        Serial.print("flag1 Adding to rpn : ");
        Serial.println(stack[stackTop]);
        rpn[index++] = stack[stackTop--];
      }
      if (stackTop >= 0) { 
        stackTop--;
      } // Pop the '('
      // pop unary operator if on stack
      if (stackTop >= 0 && isUnary(stack[stackTop])) {
        Serial.print("Adding to rpn : ");
        Serial.println(stack[stackTop]);
        rpn[index++] = stack[stackTop--];
      }
    } else if (isOp(ch)) { // normal operators
      while (stackTop >= 0 && precedence(stack[stackTop]) >= precedence(ch) && stack[stackTop] != '(') {
        Serial.print("flag2 Adding to rpn : ");
        Serial.println(stack[stackTop]);
        rpn[index++] = stack[stackTop--];
      }
      stack[++stackTop] = ch;
    }
  }
  // pop remaining operators
  while (stackTop >= 0) {
    rpn[index++] = stack[stackTop--];
  }
  // i don thtink it needs this
  rpn[index] = '\0';

  Serial.println(rpn);
  return evalRPN();
}
// returns answer from rpn
double evalRPN() {
  double stack[MAX_SIZE];
  int stackTop = -1;
  double a, b, result;

  for (int i = 0; rpn[i] != '\0'; i++) {
    char ch = rpn[i];
    //definetly rewrite this later
    switch(ch) { //switch to check special ops
      case '{':
        a = stack[stackTop--];
        if (a >= 0) {
          result = sqrt(a);
          stack[++stackTop] = result;  // push result
        } else {
          Serial.println("Error: Square root of negative number");
          return NAN;
        }
        break;
      case '[':
        a = stack[stackTop--];
        if (a >= 0) {
          result = log10(a);
          stack[++stackTop] = result;
        } else {
          Serial.println("Error: log_10 of negative number");
          return NAN;
        }
        break;
      case ']':
        a = stack[stackTop--];
        if (a >= 0) {
          result = log(a);
          stack[++stackTop] = result;  // push result
        } else {
          Serial.println("Error: ln of negative number");
          return NAN;
        }
        break;
      case '@':
        a = stack[stackTop--];
        result = sin(a);
        stack[++stackTop] = result;  // push result
        break;
      case '$':
        a = stack[stackTop--];
        result = cos(a);
        stack[++stackTop] = result;  // push result
        break;
      case '&':
        a = stack[stackTop--];
        result = tan(a);
        stack[++stackTop] = result;  // push result
        break;
      case '!':
        a = stack[stackTop--]; // Pop the num
        if (a < 0) {
          Serial.println("Error: Factorial of a negative number");
          return NAN;
        }
        result = 1;
        for(int i = 1; i <= a; i++) {
          result *= i;
        } // turn it into hulk
        stack[++stackTop] = result; // Push the result
        break;
      default: //default case handles nums and basic ops
        if (ch >= 'a' && ch <= 'z') {
          int valueIndex = ch - 'a';
          // convert char to index z = pi, e = e
          stack[++stackTop] = values[valueIndex];
          // push double from values[]
        } else { // if (isOp(ch)) { // other operators
          b = stack[stackTop--];
          a = stack[stackTop--];
          switch (ch) {
            case '+':
              result = a + b;
              break;
            case '-':
              result = a - b;
              break;
            case '*':
              result = a * b;
              break;
            case '/':
              if (b != 0) {
                result = a / b;
              } else {
                Serial.println("Error: Division by zero");
                return NAN;
              }
              break;
            case '^':
              result = pow(a, b);
              break;
          }
          stack[++stackTop] = result;  // push result
        }
    }
  }
  // only double left should be answer
  return stack[stackTop];
}