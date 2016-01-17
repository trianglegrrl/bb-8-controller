# BB-8 Controller

You need the following libraries:

 - SoftwareSerial (built in)
 - LiquidCrystal (built in)
 - aJson, great JSON library (https://github.com/interactive-matter/aJson)

Hardware used:
 - DFRobot LCD Keypad Shield
 - Arduino Uno
 - Keyes_SJoys joystick (http://www.plexishop.it/en/modulo-joystick-keyes-sjoys.html)

Trying an approach suggested by https://github.com/XRobots, the controller doesn't broadcast to the robot: the robot requests controller status through a JSON `{"command": "somecommand"}` and the controller responds with its current status.