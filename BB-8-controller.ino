// Here's my LCD Keypad Shield with pinouts
// https://www.evernote.com/shard/s443/sh/9757e419-bec9-459c-b8e3-c6f2b7ee7b2e/fb8fbe5e73d3ca6749ab9b87808ce482
// D0/1/2/3 are offset, and there is some weirdness with available pins on the top right header. See diagram for
// details.
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>

#include <aJSON.h> // https://github.com/interactive-matter/aJson

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

int joystickXPin = A3;
int joystickYPin = A2;
int joystickButtonPin = 13;

// define some values used by the buttons
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

// and some variables for the joystick and buttons
int analogKeyPressVal  = 0;
int joystickXValue = 0;
int joystickYValue = 0;
int lcdKey = 0;
  
int bluetoothTx = 2;  // TX-O pin of bluetooth mate, Arduino D2
int bluetoothRx = 3;  // RX-I pin of bluetooth mate, Arduino D3

SoftwareSerial bluetooth(bluetoothTx, bluetoothRx);

String eulerData;

aJsonStream serialStream(&Serial);
aJsonStream bluetoothStream(&bluetooth);

aJsonObject *createCommandMessage(char *command) {
  aJsonObject *msg = aJson.createObject();

  aJson.addStringToObject(msg, "command", command);
  aJson.addNumberToObject(msg, "analogKeyPressVal", analogKeyPressVal);
  aJson.addNumberToObject(msg, "joystickXValue", joystickXValue);
  aJson.addNumberToObject(msg, "joystickYValue", joystickYValue);
  aJson.addNumberToObject(msg, "lcdKey", lcdKey);
  
  return msg;
}

void issueCommand(char *cmd) {
  aJsonObject *msg = createCommandMessage(cmd);
  aJson.print(msg, &bluetoothStream);
  aJson.print(msg, &serialStream);
  aJson.deleteItem(msg);
}

int readLCDButton()
{
 analogKeyPressVal = analogRead(0);      // read the value from the sensor
 // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
 // we add approx 50 to those values and check to see if we are close
 if (analogKeyPressVal > 1000) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result
 // For V1.1 us this threshold
// if (analogKeyPressVal < 50)   return btnRIGHT;
// if (analogKeyPressVal < 250)  return btnUP;
// if (analogKeyPressVal < 450)  return btnDOWN;
// if (analogKeyPressVal < 650)  return btnLEFT;
// if (analogKeyPressVal < 850)  return btnSELECT;

 // For V1.0 comment the other threshold and use the one below:
 if (analogKeyPressVal < 50)   return btnRIGHT;
 if (analogKeyPressVal < 195)  return btnUP;
 if (analogKeyPressVal < 380)  return btnDOWN;
 if (analogKeyPressVal < 555)  return btnLEFT;
 if (analogKeyPressVal < 790)  return btnSELECT;

 return btnNONE;  // when all others fail, return this...
}

char *getCommand() {
  joystickXValue = analogRead(joystickXPin);
  joystickYValue = analogRead(joystickYPin);

  lcdKey = readLCDButton();
  
  if (lcdKey !=5) {
    return("I");
  }
  if(joystickXValue > 800) {
    return("F");
  } 
  if(joystickXValue < 250) {
    return("B");
  } 
  if(joystickYValue > 800) {
    return("R");
  } 
  if(joystickYValue < 250) {
    return("L");
  } 
  if(joystickXValue >= 490 && joystickXValue <= 600 && joystickYValue >= 490 && joystickYValue <= 600 && lcdKey == 5) {
    return("S");
  }
}

void updateLCD() {
  joystickXValue = analogRead(joystickXPin);
  joystickYValue = analogRead(joystickYPin);

  lcdKey = readLCDButton();
  readLCDButton();
  
  lcd.setCursor(0,1);
  lcd.print("                ");
  lcd.setCursor(0,1);
  lcd.print(joystickXValue);
  lcd.setCursor(5,1);
  lcd.print(joystickYValue);
  lcd.setCursor(10,1);
  lcd.print(lcdKey);
   
  lcd.setCursor(12,1);
  lcd.print(analogKeyPressVal);
}

void processRobotMessage(aJsonObject *msg) {
  aJsonObject *command = aJson.getObjectItem(msg, "command");
  issueCommand(command->valuestring);

  aJson.deleteItem(command);
}

void setup()
{
  Serial.begin(115200);
  bluetooth.begin(115200);  // The Bluetooth Mate defaults to 115200bps
  bluetooth.print("$");  // Print three times individually
  bluetooth.print("$");
  bluetooth.print("$");  // Enter command mode
  delay(100);  // Short delay, wait for the Mate to send back CMD
  bluetooth.println("U,9600,N");  // Temporarily Change the baudrate to 9600, no parity
  delay(100);
  // 115200 can be too fast at times for NewSoftSerial to relay the data reliably
  bluetooth.begin(9600);  // Start bluetooth serial at 9600
  lcd.begin(16, 2);              // start the library
  lcd.clear();
  Serial.println("Running on the controller");
}

void loop()
{  
  updateLCD();
  
  if (bluetoothStream.available()) { bluetoothStream.skip(); }

  if (bluetoothStream.available()) {
    aJsonObject *msg = aJson.parse(&bluetoothStream);
    
    processRobotMessage(msg);
    
    aJson.deleteItem(msg);
  }
  
  if (serialStream.available()) { serialStream.skip(); }

  if (serialStream.available()) {
    aJsonObject *msg = aJson.parse(&serialStream);
    
    processRobotMessage(msg);
    
    aJson.deleteItem(msg);
  }
}

