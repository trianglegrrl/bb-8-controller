
#include <LiquidCrystal.h>

#include <aJSON.h> // https://github.com/interactive-matter/aJson

#define bluetooth Serial // I'm defaulting to the Serial port on an Uno, but I'll switch to Serial1 when my new Mega comes

// used to cleanly read/write JSON data to/from the serial port(s), including bluetooth
aJsonStream bluetoothStream(&bluetooth); // bluetooth connection to the robot

/*
 * Set up library for the LCD
 */
LiquidCrystal lcd(12, 11, 7, 6, 5, 4);

#define LCD_KEYPRESS_PIN 0 // Analog pin corresponding to the keypress value
#define LCD_FIRST_LINE   0
#define LCD_SECOND_LINE  1
#define LCD_THIRD_LINE   2
#define LCD_FOURTH_LINE  3
// Display positions on the LCD screen for the given value. Screen is 2 lines, 16 characters each
#define LCD_JOYSTICK_X_POSITION       0 // Position on the LCD line that shows the joystick's X value
#define LCD_JOYSTICK_Y_POSITION       6 // Position on the LCD line that shows the joystick's Y value
#define LCD_JOYSTICK_BUTTON_POSITION 12 // Position on the LCD line that shows the button pressed

// These are the tidied values we send if these buttons are pressed
#define BUTTON_VALUE_RIGHT  0
#define BUTTON_VALUE_UP     1
#define BUTTON_VALUE_DOWN   2
#define BUTTON_VALUE_LEFT   3
#define BUTTON_VALUE_SELECT 4
#define BUTTON_VALUE_NONE   5

// Pins the joystick is connected to.
#define JOYSTICK_X_PIN      A3
#define JOYSTICK_Y_PIN      A2

/*
 * These are the thresholds above and below which the robot is commanded to move.
 * If the value is between these numbers, the joystick is in a neutral(-ish)
 * position and the command should be "stop moving!"
 */
#define JOYSTICK_X_FWD_ACTIVATE_THRESHOLD 800
#define JOYSTICK_X_BWD_ACTIVATE_THRESHOLD 250
#define JOYSTICK_Y_FWD_ACTIVATE_THRESHOLD 800
#define JOYSTICK_Y_BWD_ACTIVATE_THRESHOLD 250

// and some variables for the joystick and buttons
int analogKeyPressVal  = 0; // The raw value that the LCD Keypad Shield returns for a button press
int lcdKeyPressed = 0; // The BUTTON_VALUE_* (see above) of the key that was pressed

int joystickXValue = 0; // The analog value of the joystick X position (0-1024)
int joystickYValue = 0; // The analog value of the joystick Y position (0-1024)

/* =================================================================
 * setup() - Runs once on startup
 */
void setup() {
  Serial.begin(115200); // Local (USB) serial port.

  setupBlueSMiRF(); // Bluetooth adapter

  setupLCDShield(); // DFRobot LCD Keypad shield

//  Serial.println("BB-8 Controller ready.");
}

/* =================================================================
 * loop() - Runs forever
 */
void loop() {
  // Update the LCD with whatever I want to display
  updateLCD();

  // Read command from robot via Bluetooth adapter
  readAndProcessRobotMessageIfAvailable(&bluetoothStream);
}

/* =================================================================
 * createResponseToRobot() - Create the JSON-formatted response to
 * send back to the robot, as requested
 *
 * The message to the robot includes a lot of data about the state of
 * the controller. The message currently looks like this:
 *
 * {
 *   "command": "F",           // Currently a single character command that's inferred from the controller state
 *   "analogKeyPressVal": 300, // The analog value that the LCD Keypad Shield returns for the key that is pressed
 *   "joystickXValue": 506,    // The analog value of the joystick's X position
 *   "joystickYValue": 506,    // The analog value of the joystick's Y position
 *   "lcdKeyPressed": 3        // The BUTTON_VALUE_* (see above) of the key that was pressed
 * }
 */
aJsonObject *createResponseToRobot(char *command) {
  aJsonObject *msg = aJson.createObject();

  aJson.addStringToObject(msg, "command", command); // Currently a single character command that's inferred from the controller state
  aJson.addNumberToObject(msg, "analogKeyPressVal", analogKeyPressVal); // The analog value that the LCD Keypad Shield returns for the key that is pressed
  aJson.addNumberToObject(msg, "joystickXValue", joystickXValue); // The analog value of the joystick's X position
  aJson.addNumberToObject(msg, "joystickYValue", joystickYValue); // The analog value of the joystick's Y position
  aJson.addNumberToObject(msg, "lcdKeyPressed", lcdKeyPressed); // The BUTTON_VALUE_* (see above) of the key that was pressed

  return msg;
}

/* =================================================================
 * handleCommand() - Do something special with the incoming command
 * TODO: Right now the command is always "?", which means "send me your
 * controller state". It should be able to do other stuff, but I don't
 * need it to do other stuff right now.
 */
void handleCommand(char *cmd) {
  aJsonObject *msg = createResponseToRobot(inferCommandToIssueFromControllerState());

  aJson.print(msg, &bluetoothStream); // Send the json to the robot
  bluetooth.println(" ");
  aJson.deleteItem(msg); // Clean up allocated memory
}

/* =================================================================
 * readLCDButton() - Set the current analog value of the key that is
 * pressed and return the key pressed, BUTTON_VALUE_NONE if none.
 *
 * I'm using the DFRobot LCD Keypad Shield right now, but I don't like
 * it and hope to replace it with a 2.8" TFT screen soon.
 */
int readLCDButton() {
 analogKeyPressVal = analogRead(LCD_KEYPRESS_PIN); //

 // Values are for v1.0 of the LCD keypad shield. Inelegant but functional.
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
 */
char *inferCommandToIssueFromControllerState() {
  joystickXValue = analogRead(JOYSTICK_X_PIN);
  joystickYValue = analogRead(JOYSTICK_Y_PIN);

  lcdKeyPressed = readLCDButton();

  if (isJoystickInStopPosition(joystickXValue, joystickYValue)) { return("BS"); }

  if (joystickXValue > JOYSTICK_X_FWD_ACTIVATE_THRESHOLD) { return("BF"); }
  if (joystickXValue < JOYSTICK_X_BWD_ACTIVATE_THRESHOLD) { return("BB"); }
  if (joystickYValue > JOYSTICK_Y_FWD_ACTIVATE_THRESHOLD) { return("BR"); }
  if (joystickYValue < JOYSTICK_Y_BWD_ACTIVATE_THRESHOLD) { return("BL"); }

  if (lcdKeyPressed == BUTTON_VALUE_LEFT) { return("PHL"); }
  if (lcdKeyPressed == BUTTON_VALUE_RIGHT) { return("PHR"); }
  if (lcdKeyPressed == BUTTON_VALUE_UP) { return("THL"); }
  if (lcdKeyPressed == BUTTON_VALUE_DOWN) { return("THR"); }

  return("BS"); // Default: all stop.
}

/* =================================================================
 * isJoystickInStopPosition() - Returns true if joystick is in a "stop"
 * position, false otherwise.
 */
bool isJoystickInStopPosition(int joystickXValue, int joystickYValue) {
  return(
        joystickXValue >= JOYSTICK_X_BWD_ACTIVATE_THRESHOLD
     && joystickXValue <= JOYSTICK_X_FWD_ACTIVATE_THRESHOLD
     && joystickYValue >= JOYSTICK_Y_BWD_ACTIVATE_THRESHOLD
     && joystickYValue <= JOYSTICK_Y_FWD_ACTIVATE_THRESHOLD
  );
}

/* =================================================================
 * shittyHackToSendStatusIfYouPressedSelect() - Descriptive function names FTW.
 *
 * PELIGRO! todo: This is a hack to make the controller send a message
 * to the robot containing its current state. Used for testing.
 * Should never need it in production. Delete this once the robot works.
 *
 * AH, 19 Jan 2016
 */
void shittyHackToSendStatusIfYouPressedSelect(int lcdKeyPressed) {
  // TODO: Refactor this for proper button handling
  // Temp hack to use this button to send state.
  if (lcdKeyPressed == BUTTON_VALUE_SELECT) {
//    Serial.println("You pressed the select button");
    aJsonObject *msg = createResponseToRobot("M");

    aJson.print(msg, &bluetoothStream);
//    aJson.print(msg, &serialStream);

    aJson.deleteItem(msg);
  }
}

/* =================================================================
 * updateLCD() - Update the LCD with the values we want to display.
 *
 * LCD Panel - 16 characters long, 2 lines
 * QUn - Quaternion value from IMU (WXYZ)
 * Xjoyv - Joystick's X analog value
 * Yjoyv - Joystick's Y analog value
 * BUTN - Text of which button was pressed

 * Position 0123456789ABCDEF
 * LINE1:   QU1 QU2 QU3 QU4     - Quaternion values from robot IMU
 * LINE2:   Xjoyv Yjoyv BUTN    - Values from controller
 */
void updateLCD() {
  static int lastJoystickXValue = 0;
  static int lastJoystickYValue = 0;
  static int lastLcdKeyPressed = 0;

  // Set global variables with the values
  joystickXValue = analogRead(JOYSTICK_X_PIN);
  joystickYValue = analogRead(JOYSTICK_Y_PIN);
  lcdKeyPressed = readLCDButton();

  // todo: remove this, but remove it because it's stupid.
  shittyHackToSendStatusIfYouPressedSelect(lcdKeyPressed);

  // Only update if this value has changed
  if(lastJoystickXValue != joystickXValue) {
    // My hacky way to clear only the area I want
    lcd.setCursor(LCD_JOYSTICK_X_POSITION, LCD_SECOND_LINE); lcd.print("    ");
    // Update the value
    lcd.setCursor(LCD_JOYSTICK_X_POSITION, LCD_SECOND_LINE); lcd.print("X"); lcd.print(joystickXValue);

    lastJoystickXValue = joystickXValue;
  }

  // Only update if this value has changed
  if(lastJoystickYValue != joystickYValue) {
    // My hacky way to clear only the area I want
    lcd.setCursor(LCD_JOYSTICK_Y_POSITION, LCD_SECOND_LINE); lcd.print("    ");
    // Update the value
    lcd.setCursor(LCD_JOYSTICK_Y_POSITION, LCD_SECOND_LINE); lcd.print("Y"); lcd.print(joystickYValue);

    lastJoystickYValue = joystickYValue;
  }

  // Only update if this value has changed
  if(lastLcdKeyPressed != lcdKeyPressed) {
    // My hacky way to clear only the area I want
    lcd.setCursor(LCD_JOYSTICK_BUTTON_POSITION, LCD_SECOND_LINE); lcd.print("    ");
    // Update the value
    lcd.setCursor(LCD_JOYSTICK_BUTTON_POSITION, LCD_SECOND_LINE); lcd.print(lcdButtonName(lcdKeyPressed));

    lastLcdKeyPressed = lcdKeyPressed;
  }
}

/* =================================================================
 * lcdButtonName() - Return readable name for the specified button.
 */
char *lcdButtonName(int keyVal) {
  switch(keyVal) {
    case BUTTON_VALUE_RIGHT:
      return "RIGHT";
    case BUTTON_VALUE_UP:
      return "UP";
    case BUTTON_VALUE_DOWN:
      return "DOWN";
    case BUTTON_VALUE_LEFT:
      return "LEFT";
    case BUTTON_VALUE_SELECT:
      return "SELECT";
    case BUTTON_VALUE_NONE:
      return "NONE";
    default:
      return "UNKNOWN";
  }
}

/* =================================================================
 * readAndProcessRobotMessageIfAvailable() - Check the specified
 * stream for JSON from the robot, and if there is some, process the
 * data and react accordingly.
 */
void readAndProcessRobotMessageIfAvailable(aJsonStream *stream) {
  if (stream->available()) { stream->skip(); }

  if (stream->available()) {
    aJsonObject *msg = aJson.parse(stream);

    processRobotMessage(msg);

    aJson.deleteItem(msg);
  }
}

/* =================================================================
 * processRobotMessage() - Parse and process the received message.
 * Right now, all we're looking for is the command the robot is
 * sending. See handleCommand() for more details.
 */
void processRobotMessage(aJsonObject *msg) {
  aJsonObject *command = aJson.getObjectItem(msg, "command");

  handleCommand(command->valuestring);

  aJson.deleteItem(command);
}

/* =================================================================
 * setupBlueSMiRF() - Configure BlueSMiRF for commanding
 */
void setupBlueSMiRF() {
  bluetooth.begin(115200);
}

/* =================================================================
 * setupLCDShield() - Configure LCD shield
 */
void setupLCDShield() {
  lcd.begin(20, 4); // 4 lines, 20 characters each
  lcd.clear();
}
