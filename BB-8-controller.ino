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
#define BUTTON_VALUE_RIGHT  0
#define BUTTON_VALUE_UP     1
#define BUTTON_VALUE_DOWN   2
#define BUTTON_VALUE_LEFT   3
#define BUTTON_VALUE_SELECT 4
#define BUTTON_VALUE_NONE   5

// and some variables for the joystick and buttons
int analogKeyPressVal  = 0;
int joystickXValue = 0;
int joystickYValue = 0;
int lcdKeyPressed = 0;

int bluetoothTx = 2;  // TX-O pin of bluetooth mate, Arduino D2
int bluetoothRx = 3;  // RX-I pin of bluetooth mate, Arduino D3

SoftwareSerial bluetooth(bluetoothTx, bluetoothRx);

aJsonStream serialStream(&Serial);
aJsonStream bluetoothStream(&bluetooth);

/* =================================================================
 * createResponseToRobot() - Create the JSON-formatted response to
 * send back to the robot, as requested
 *
 */
aJsonObject *createResponseToRobot(char *command) {
  aJsonObject *msg = aJson.createObject();

  aJson.addStringToObject(msg, "command", command);
  aJson.addNumberToObject(msg, "analogKeyPressVal", analogKeyPressVal);
  aJson.addNumberToObject(msg, "joystickXValue", joystickXValue);
  aJson.addNumberToObject(msg, "joystickYValue", joystickYValue);
  aJson.addNumberToObject(msg, "lcdKeyPressed", lcdKeyPressed);

  return msg;
}

/* =================================================================
 * handleCommand() - Do something special with the incoming command
 *
 */
void handleCommand(char *cmd) {
  // TODO: Right now the command is always "?", which means "send me your controller state".
  // It should be able to do other stuff, but I don't need it to do other stuff right now..
  aJsonObject *msg = createResponseToRobot(inferCommandToIssueFromControllerState());

  aJson.print(msg, &bluetoothStream);
  aJson.print(msg, &serialStream);

  aJson.deleteItem(msg);
}

/* =================================================================
 * readLCDButton() - Set the current analog value of the key that is
 * pressed and return the key pressed, BUTTON_VALUE_NONE if none.
 *
 */
int readLCDButton() {
 analogKeyPressVal = analogRead(0);

 if (analogKeyPressVal > 1000) { return BUTTON_VALUE_NONE; }
 if (analogKeyPressVal < 50)   { return BUTTON_VALUE_RIGHT; }
 if (analogKeyPressVal < 195)  { return BUTTON_VALUE_UP; }
 if (analogKeyPressVal < 380)  { return BUTTON_VALUE_DOWN; }
 if (analogKeyPressVal < 555)  { return BUTTON_VALUE_LEFT; }
 if (analogKeyPressVal < 790)  { return BUTTON_VALUE_SELECT; }

 return BUTTON_VALUE_NONE;
}

/* =================================================================
 * inferCommandToIssueFromControllerState() - Given the controller's
 * current state, infer the command to issue to the robot. This is
 * only useful until I make the robot smart enough to make decisions
 * based on the controller's raw values. Or maybe I will leave it
 * like this forever. Who knows?
 *
 */
char *inferCommandToIssueFromControllerState() {
  joystickXValue = analogRead(joystickXPin);
  joystickYValue = analogRead(joystickYPin);

  lcdKeyPressed = readLCDButton();

  if (lcdKeyPressed !=5) { return("I"); }
  if(joystickXValue > 800) {return("F"); }
  if(joystickXValue < 250) {return("B"); }
  if(joystickYValue > 800) {return("R"); }
  if(joystickYValue < 250) {return("L"); }

  if(joystickXValue >= 490 && joystickXValue <= 600 && joystickYValue >= 490 && joystickYValue <= 600 && lcdKeyPressed == 5) {
    return("S");
  }
}

/* =================================================================
 * updateLCD() - Update the LCD with the values we want to display.
 *
 */
void updateLCD() {
  joystickXValue = analogRead(joystickXPin);
  joystickYValue = analogRead(joystickYPin);

  lcdKeyPressed = readLCDButton();
  readLCDButton();

  lcd.setCursor(0,1);
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print(joystickXValue);
  lcd.setCursor(5,1);
  lcd.print(joystickYValue);
  lcd.setCursor(10,1);
  lcd.print(lcdKeyPressed);

  lcd.setCursor(12,1);
  lcd.print(analogKeyPressVal);
}

void processRobotMessage(aJsonObject *msg) {
  aJsonObject *command = aJson.getObjectItem(msg, "command");

  handleCommand(command->valuestring);

  aJson.deleteItem(command);
}

/* =================================================================
 * setup() - Runs once on startup
 *
 */
void setup() {
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

/* =================================================================
 * loop() - Runs forever
 *
 */
void loop() {
  // Update the LCD with whatever I want to display
  updateLCD();

  // Process commands from the Bluetooth stream
  if (bluetoothStream.available()) { bluetoothStream.skip(); }

  if (bluetoothStream.available()) {
    aJsonObject *msg = aJson.parse(&bluetoothStream);

    processRobotMessage(msg);

    aJson.deleteItem(msg);
  }

  // Also process commands from the serial port stream - super handy for troubleshooting
  if (serialStream.available()) { serialStream.skip(); }

  if (serialStream.available()) {
    aJsonObject *msg = aJson.parse(&serialStream);

    processRobotMessage(msg);

    aJson.deleteItem(msg);
  }
}

