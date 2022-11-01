#include <Process.h>
#include <stdio.h>
// Add BLE Gamepad
#include <BleGamepad.h>  
// Add OLED
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// Add Adafruit Sensor Libs for Tilt Controls
#include <Adafruit_Sensor.h>
#include <Adafruit_MPU6050.h>

//====================================================
// __________.____     ___________.________ _________   
// \______   \    |    \_   _____/ \_____  \\______  \  
//  |    |  _/    |     |    __)_    _(__  < |    |   \ 
//  |    |   \    |___  |        \  /       \|    `    \
//  |______  /_______ \/_______  / /______  /________  /
//         \/        \/        \/         \/         \/ 
// Greetz and massive thanks to Hexfreq for being a complete bad ass
//
// All reliable controller code foundation is from [at]Hexfreq
// All bugs, poor code choices, and existing PCB designs courtesy of [at]GamingNJncos
// Cheers to [at]Arthrimus for lighting this fire on accident
//
// As I have limited time for mainting this codebase long term attempts have been made to over document code provided
// No Warranty expressed or implied
//====================================================


// OLED Screen Settings
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Establish tasks for ESP32 CPU Core Pinning 
TaskHandle_t Task1;
TaskHandle_t Task2;

// BLEGamePad Settings
// ===================
// Values are Advertised Name, Manufacturer, Battery level 
// Note the battery value is not from a read value here but later blegamepad examples support live update vs 1 time poll
BleGamepad bleGamepad("Reala", "GamingNJncos.HexFreq", 100);
// Purposely set high to permit ease of future additional button combinationsfor to send Meta, home button, etc  
#define numOfButtons 14 
#define numOfHatSwitches 1 //DPAD (not Analog pad)
#define enableX true // Analog Pad
#define enableY true // Analog Pad
#define enableRX true // Triggers
#define enableRY true // Triggers
// Additional button options currently disabled
#define enableZ false
#define enableRZ false
#define enableSlider1 false
#define enableSlider2 false
#define enableRudder false
#define enableThrottle false
#define enableAccelerator false
#define enableBrake false
#define enableSteering false
// End BLEGamePad Settings

// Button State Tracking
// This nets a significant advantage in performance vs not tracking states
// BLE expects ACK per packet so flooding "not pressed" or identical data can add extreme delays
// 
byte previousButtonStates[14];
byte currentButtonStates[14];
//  Button State Array Map
//  ======================
//  MODE,   DPAD,   START, A, B, C, X, Y, Z,     L, R,       Analog PAD
//  [0-1]   [0-8]   [-  Binary Values 0-1 -]     [0-15]     [0-127,0-127]
//  Note Left and Right stick in digital mode will return Binary [0-1] vs [0-15] value in analog mode

// Pinmappings for your PCB version
// It is manditory you select the correct pinout for your board type

// Feather HeavyWing to Controller Pin Mapping
// ===========================================
//PCB: https://oshpark.com/shared_projects/ki7HbZV4
//
//int dataPinD0 = 32; //  Arthrimus Pin 3 = Data D0
//int dataPinD1 = 14; //  Arthrimus Pin 2 = Data D1
//int dataPinD2 = 13; //  Arthrimus Pin 8 = Data D2
//int dataPinD3 = 12; //  Arthrimus Pin 7 = Data D3
//int THPin = 15; // Arthrimus Pin 4 = TH Select
//int TRPin = 33; // Arthrimus Pin 5 = TR Request
//int TLPin = 27;  //    Arthrimus Pin 6 = TL Response
//int batPin = A13; //  Used by BLEGamepad library
//int latPin =  4;   //  Latency testing pin
// End Feather HeavyWing to Controller Pin Mapping


// Feather LightWing to Controller Pin Mapping
// ===========================================
// PCB: https://oshpark.com/shared_projects/7RqwDWJT
//
int dataPinD0 = 12; //  Arthrimus Pin 3 = Data D0
int dataPinD1 = 13; //  Arthrimus Pin 2 = Data D1
int dataPinD2 = 14; //  Arthrimus Pin 8 = Data D2
int dataPinD3 = 32; //  Arthrimus Pin 7 = Data D3
int THPin = 27; // Arthrimus Pin 4 = TH Select
int TRPin = 33; // Arthrimus Pin 5 = TR Request
int TLPin = 15;  //    Arthrimus Pin 6 = TL Response
int batPin = A13; //  Used by BLEGamepad library
int latPin =  4;   //  Latency testing pin
// End Feather HeavyWing to Controller Pin Mapping
// End all Pin Mappings

// BleGamepad Variables
// ====================
int16_t bleAnalogX;  //Analog pad X Axis Output
int16_t bleAnalogY;  //Analog pad Y Axis Output
// DeadZone defined as smallest value to exceed before sending in blegamepad report function
int16_t bleDeadZone = 400; //Set Deadzone via minimum number 
// Analog Triggers
int16_t bleRTrigger; //Right Trigger Analog Output
int16_t bleLTrigger; //Left Trigger Analog Output
// Battery Monitoring variables for BLEGamepad
// Note BLE Gamepad can read battery percent but can not check to transmit value after initial connection
int adcRead = 0;
int batVolt = 0;
// End BleGamepad Variables


// Controller protocol and timing variables
// ========================================
// Data Array for each byte from protocol
boolean dataArrayBit0[8];
boolean dataArrayBit1[8];
boolean dataArrayBit2[8];
boolean dataArrayBit3[8];
boolean dataArrayBit4[8];
boolean dataArrayBit5[8];
boolean dataArrayBit6[8];
boolean dataArrayBit7[8];
// Assorted Protocol Variables
int nibble0Read = 0;  // Confirm 1st Nibble has been read
int byteCounter = 0;  // Current Byte Tracker
int modeCounter = 4;  // Reference for which mode we are currently in Analogue or Digital
int DecodeData = 1;   // Controller Successfully Decoded Data Variable
int curMode = 0;      // This stores the Controller Mode to pass between ESP cores
// Assorted timing oriented variables
unsigned long timeStamp = 0;            // Store time of last update
unsigned long lastTimeStamp = 0;        // Store Last time for comparison
const long interval = 1000;             // Data update check timing
unsigned long currentMillis = millis(); // check timing
// Directional Variables
int upDownValue = 0;
int leftRightValue = 0;
// Trigger Variables
int ltValue = 0;
int rtValue = 0;
// Variables for Data send request and Acknowledgments
int THSEL = 0;
int TRREQ = 0;
int TLACK = 0;
// Sets the number of Bytes required for Digital or Analogue Modes
int AnalogMode = 6;
int DigitalMode = 3;
// End Controller protocol and timing variables


// Gyro and Accelerometer Variables
// =================================
// Gyro is currently disabled in code but my terrible examples should be useful if you wish to use this feature
const int MPU_addr=0x68;               // Define Gyro I2C Address 
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;   // Store all values from MPU
int minVal=265;
int maxVal=402;
double x;
double y;
double z;
// Converted XY Values to BLEGamepad ranges (negative to positive 32K
int xmap;
int ymap;
// Max Values to limit full range of 360 degree motion.
// Will require calibration with your MPU-6050 to identify most comfortable ranges
// Suggest 35-45 Degrees or less relatively comfortable for most use cases
int xmax = 200;
int ymax = 200;
int xmin = 160;
int ymin = 160;
// End Gyro and Accelerometer Variables


//  Boot Logo
// ==========
//Set Image Size for Boot Logo
#define imageWidth 128
#define imageHeight 64
// Image to be displayed
// These can be changed and alternatively set as animations
// Create with "LCD Image Converter" tool or similar
// https://lcd-image-converter.riuson.com/en/about/
const unsigned char myBitmap [] PROGMEM=
{
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x0F, 0xFE, 0x0C, 0x00, 0xF6, 0x0C, 0x3C, 0x00, 0x60, 0x41, 0x80, 0x87, 0xFF, 0xF1, 0x80, 0x1E,
0x1E, 0x3F, 0xFB, 0xE7, 0x9E, 0x0E, 0x3E, 0x00, 0xE7, 0xBE, 0x7F, 0x03, 0xBF, 0x1E, 0xE0, 0x0E,
0x3C, 0x03, 0xE0, 0x7E, 0x03, 0x1E, 0x3E, 0x00, 0xE4, 0x1E, 0x0F, 0x03, 0x1E, 0x1E, 0x78, 0x0C,
0x3E, 0x01, 0xE0, 0x1E, 0x01, 0x1E, 0x3E, 0x01, 0xF0, 0x1E, 0x0F, 0x03, 0x1E, 0x0F, 0x3C, 0x0C,
0x3F, 0x01, 0xE1, 0x3E, 0x00, 0x3F, 0x1F, 0x03, 0xF0, 0x1E, 0x0F, 0x03, 0x1E, 0x0F, 0x3F, 0x0C,
0x1F, 0xC1, 0xE3, 0x3C, 0x00, 0x3D, 0x1F, 0x03, 0xD8, 0x1E, 0x0F, 0x03, 0x1E, 0x1E, 0x3F, 0x8C,
0x0F, 0xF1, 0xFF, 0x3C, 0x00, 0x79, 0x9F, 0x87, 0xD8, 0x1E, 0x0F, 0x03, 0x1E, 0x3E, 0x3F, 0xCC,
0x03, 0xF9, 0xFF, 0x3C, 0x00, 0xF9, 0x8F, 0x87, 0x8C, 0x1E, 0x0F, 0x03, 0x1E, 0xF0, 0x27, 0xEC,
0x00, 0xFD, 0xE3, 0x3C, 0x1F, 0xF0, 0xCF, 0xCF, 0x8C, 0x1E, 0x0F, 0x03, 0x1E, 0x78, 0x31, 0xFC,
0x40, 0x7D, 0xE0, 0x3E, 0x0F, 0xF0, 0xC7, 0xCF, 0x06, 0x1E, 0x0F, 0x03, 0x1E, 0x78, 0x30, 0xFC,
0x60, 0x3D, 0xE0, 0x3E, 0x07, 0xE0, 0x67, 0xDF, 0x06, 0x1E, 0x0F, 0x03, 0x1E, 0x3C, 0x30, 0x7C,
0x30, 0x3D, 0xE0, 0x7E, 0x07, 0xE0, 0x63, 0xFE, 0x03, 0x1E, 0x07, 0x87, 0x1E, 0x3E, 0x30, 0x3C,
0x3E, 0xF9, 0xF3, 0xFF, 0x0F, 0xE0, 0x73, 0xFE, 0x07, 0xBE, 0x07, 0xFE, 0x1F, 0x1F, 0x30, 0x0C,
0x1F, 0xE3, 0xFF, 0xE3, 0xFF, 0xE0, 0xFB, 0xFE, 0x07, 0xFF, 0x01, 0xF8, 0x3F, 0x87, 0xF0, 0x04,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// End Boot Logo


// Boot Logo and Screen Setup
// ==========================
void bootLogo(){
  //Check ESP Core Boot Logo is running on
  //Serial.print("BootLogo Core ");
  //Serial.println(xPortGetCoreID());

   if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  // Rotate
  display.setRotation(2);
  // Clear the Buffer
  display.clearDisplay(); 
  // Draw Animation
  // Draw Bitmap syntax
  // display.drawBitmap(x position, y position, bitmap data, bitmap width, bitmap height, color)
  display.drawBitmap(0, 32, myBitmap, 128, 32, WHITE); 
  display.display();
  // Set Delay for Boot Animation
  delay(1500);
  // Reset Display
  display.display();
  display.setRotation(2);
  delay(10); 
  // Clear the buffer
  display.clearDisplay();
  delay(4000);
  //End Screen Setup
  }


// Battery Related Checks
// ======================
void getBatStat(){
  // Check ESP Core
  //Serial.print("Battery Status Core ");
  //Serial.println(xPortGetCoreID());

  adcRead = analogRead(batPin);
  batVolt = adcRead;
  // Multiply by 2 to correct reading for Featherwing Huzzah32
  //batVolt = adcRead * 2;

  int BatteryPct;
  int oldBatPct = 0;
  // Map Battery voltage range to a percentage
  BatteryPct = map (batVolt, 1200, 4200, 0, 100);
  // Make sure its populated on initial run
  if (oldBatPct < 1){
    oldBatPct = BatteryPct;
  }
  // Super hack check to omit negative values from OLED Updates
  if (BatteryPct < 0 ){
    BatteryPct = oldBatPct;
  }
  
  // Build and send screen update
  // ============================
  String lcdBat;
  lcdBat = "Battery: " + String(BatteryPct) + "%    ";
  display.setCursor(0,50);
  display.print("Battery  ");
  display.setCursor(0,50);
  display.print(lcdBat);
  display.display();
  // End Battery Updates
  }

// OLED Updates
// ============
void oled_Updates(){
if (curMode== 0 ) {
  curMode = 0;
  display.setTextSize(1);                       // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE, BLACK);   // Draw white text
  display.setCursor(0,1);                       // Start at top-left corner
  display.println(F("Mode: Analog "));
  display.display();
  } else {
  curMode = 1;
  display.setTextSize(1);                       // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE, BLACK);   // Draw white text
  display.setCursor(0,1);                       // Start at top-left corner
  display.println(F("Mode: Digital "));
  display.display();
    }
  // End OLED Updates
  }


// Controller related items
// ========================
void pollController(){
  //Check ESP Core Controller Polling Occurs on
  //Serial.print("Controller Polling Core ");
  //Serial.println(xPortGetCoreID());

  //Make sure that we sync timestamp and last timestamp on first run
  if (lastTimeStamp = 0){
    lastTimeStamp = timeStamp;
    //hacktest MPU initialization on first run
     //initializeSensor();
  }
  
  // Start The Data Read
  // Is the Controller ready to send the data?
  TLACK = digitalRead(TLPin);

  // Have we read all the bytes needed for each mode?
  if (byteCounter > modeCounter) {
      // Set Not ready for Data Coms
      digitalWrite(THPin, HIGH);
      digitalWrite(TRPin, HIGH);

  // Are we using Analogue Mode or Digital?
  if (modeCounter == AnalogMode) {
        curMode = 0;
        currentButtonStates[0] = 0;
      } else {
        curMode = 1;
        currentButtonStates[0] = 1;
      }
      
  // Decoded Controller Data Successfully
  // Proceed based on Analog or Digital Mode
  //
  // Note that BLEgamepad check occurs in combination with successfully decoded data
  // This avoids a bug in NIMBLE where blegamepad could under some circumstances attempt button send prior to connecting and cause abort
  if (DecodeData == 1 && bleGamepad.isConnected()) {
    //Analog
    if (modeCounter == AnalogMode) {
      //Analog Mode - Process Input
      //Begin all Analog Mode data processing
      
      //DPAD
      if (dataArrayBit6[1] != 0 && dataArrayBit7[1] != 0 && dataArrayBit5[1] != 0 && dataArrayBit4[1] != 0){
        bleGamepad.setHat1(DPAD_CENTERED);
        currentButtonStates[1] = 0;
      }
      if (dataArrayBit4[1] == 0 && (dataArrayBit7[1] != 0 && dataArrayBit6[1] != 0)) {
        //Serial.print(" U");
        bleGamepad.setHat1(DPAD_UP);
        currentButtonStates[1] = 1;
      }
      if (dataArrayBit5[1] == 0 && (dataArrayBit7[1] != 0 && dataArrayBit6[1] != 0)) {
        //Serial.print(" D");
        bleGamepad.setHat1(DPAD_DOWN);
        currentButtonStates[1] = 2;
      }
      if (dataArrayBit6[1] == 0 && (dataArrayBit4[1] != 0 && dataArrayBit5[1] != 0)) {
       //Serial.print(" L");
        bleGamepad.setHat1(DPAD_LEFT);
        currentButtonStates[1] = 3;
      }
      if (dataArrayBit7[1] == 0 && (dataArrayBit4[1] != 0 && dataArrayBit5[1] != 0)) {
        //Serial.print(" R");
        bleGamepad.setHat1(DPAD_RIGHT);
        currentButtonStates[1] = 4;
      }
      if (dataArrayBit4[1] == 0 && dataArrayBit6[1] == 0) {
        //Serial.print(" U/L");
        bleGamepad.setHat1(DPAD_UP_LEFT);
        currentButtonStates[1] = 5;
      }
      if (dataArrayBit4[1] == 0 && dataArrayBit7[1] == 0) {
        //Serial.print(" U/R");
        bleGamepad.setHat1(DPAD_UP_RIGHT);
        currentButtonStates[1] = 6;
      }
    if (dataArrayBit5[1] == 0 && dataArrayBit6[1] == 0) {
      //Serial.print(" D/L");
      bleGamepad.setHat1(DPAD_DOWN_LEFT);
      currentButtonStates[1] = 7;
    }
    if (dataArrayBit5[1] == 0 && dataArrayBit7[1] == 0) {
      //Serial.print(" D/R");
      bleGamepad.setHat1(DPAD_DOWN_RIGHT);
      currentButtonStates[1] = 8;
    }
    // End DPAD

    //Buttons
      if (dataArrayBit2[2] == 0) {
        //Serial.print(" A");
        bleGamepad.press(BUTTON_4);
        currentButtonStates[3] = 1;
      } else {
        //Serial.print(" ");
        bleGamepad.release(BUTTON_4);
        currentButtonStates[3] = 0;
      }
      if (dataArrayBit0[2] == 0) {
        //Serial.print(" B");
        bleGamepad.press(BUTTON_1);
        currentButtonStates[4] = 1;
      } else {
        //Serial.print(" ");
        bleGamepad.release(BUTTON_1);
        currentButtonStates[4] = 0;
      }
      if (dataArrayBit1[2] == 0) {
        //Serial.print(" C");
        bleGamepad.press(BUTTON_2);
        currentButtonStates[5] = 1;
      } else {
        //Serial.print(" ");
        bleGamepad.release(BUTTON_2);
        currentButtonStates[5] = 0;
      }
      if (dataArrayBit6[2] == 0) {
        //Serial.print(" X");
        bleGamepad.press(BUTTON_7);
        currentButtonStates[6] = 1;
      } else {
        //Serial.print(" ");
        bleGamepad.release(BUTTON_7);
        currentButtonStates[6] = 0;
      }
      if (dataArrayBit5[2] == 0) {
      // Serial.print(" Y");
        bleGamepad.press(BUTTON_5);
        currentButtonStates[7] = 1;
      } else {
        //Serial.print(" ");
        bleGamepad.release(BUTTON_5);
        currentButtonStates[7] = 0;
      }
      if (dataArrayBit4[2] == 0) {
        //Serial.print(" Z");
        bleGamepad.press(BUTTON_8);
        currentButtonStates[8] = 1;
      } else {
        //Serial.print(" ");
        bleGamepad.release(BUTTON_8);
        currentButtonStates[8] = 0;
      }
      // Uncomment for L/R triggers to send digital in addition to analog values
      // May have wrong button_number defined here in the blegamepad.press setting
      //if (dataArrayBit3[3] == 0) {
        //Serial.print(" LT");
      //  bleGamepad.press(BUTTON_5);
      //  currentButtonStates[9] = 1;
      //} else {
        //Serial.print(" ");
      //  bleGamepad.release(BUTTON_5);
      //  currentButtonStates[9] = 0;
      //}
      //if (dataArrayBit7[2] == 0) {
        //Serial.print(" RT");
      //  bleGamepad.press(BUTTON_6);
      //  currentButtonStates[10] = 1;
      //} else {
        //Serial.print(" ");
      //  bleGamepad.release(BUTTON_6);
      //  currentButtonStates[10] = 1;
      //}
      if (dataArrayBit3[2] == 0) {
        //Serial.print(" Start ");
        bleGamepad.press(BUTTON_12);
        currentButtonStates[2] = 1;
      } else {
        bleGamepad.release(BUTTON_12);
        currentButtonStates[2] = 0;
      }
      // Latency Test Pin
      // Intended for BlueRetro latency button
      // Can be used for other additional buttons
      //
      // Note this sends "Start" but can be adjusted to press any other button
      if (digitalRead(latPin) == LOW) {
        //Serial.print("LAT");
        bleGamepad.press(BUTTON_4);
        currentButtonStates[3] = 1;
      } else {
        bleGamepad.release(BUTTON_4);
        currentButtonStates[3] = 0;
      }

      // Up / Down
      if (dataArrayBit7[4] == 1) {
        upDownValue = dataArrayBit0[5] + (dataArrayBit1[5]<<1) + (dataArrayBit2[5]<<2) + (dataArrayBit3[5]<<3) + (dataArrayBit4[4]<<4) + (dataArrayBit5[4]<<5) + (dataArrayBit6[4]<<6);
        bleAnalogX = map(upDownValue, 0, 127, 1, 32767);
                     
      } else {
        upDownValue = !dataArrayBit0[5] + (!dataArrayBit1[5]<<1) + (!dataArrayBit2[5]<<2) + (!dataArrayBit3[5]<<3) + (!dataArrayBit4[4]<<4) + (!dataArrayBit5[4]<<5) + (!dataArrayBit6[4]<<6);
        bleAnalogX = map(upDownValue, 1, 127, -1, -32767);
      }
      // Deadzone fix - X Axis
      // Value is checked based on map output
      if ((bleAnalogX < 0 && bleAnalogX > -500)||(bleAnalogX < 0 && bleAnalogX > -500)){
         bleAnalogX = 0;
      }


      //Analog Debugs
      //Serial.print(" bleAnalogX ");
      //Serial.println(bleAnalogX);
      // Format Value to 3 Chars long
      //char buf[] = "000";
      //sprintf(buf, "%03i", upDownValue);

      // Left / Right
      if (dataArrayBit7[3] == 1) {
        //Serial.print(" R = ");
        leftRightValue = dataArrayBit0[4] + (dataArrayBit1[4]<<1) + (dataArrayBit2[4]<<2) + (dataArrayBit3[4]<<3) + (dataArrayBit4[3]<<4) + (dataArrayBit5[3]<<5) + (dataArrayBit6[3]<<6);
        bleAnalogY = map(leftRightValue, 1, 127, 1, 32767);
      } else {
        leftRightValue = !dataArrayBit0[4] + (!dataArrayBit1[4]<<1) + (!dataArrayBit2[4]<<2) + (!dataArrayBit3[4]<<3) + (!dataArrayBit4[3]<<4) + (!dataArrayBit5[3]<<5) + (!dataArrayBit6[3]<<6);
        bleAnalogY = map(upDownValue, 1, 127, -32767, -1);
      }
      // Deadzone fix - Y Axis
      // Value is checked based on map output
      if ((bleAnalogY > 0 && bleAnalogY < 500)||(bleAnalogY < 0 && bleAnalogY > -500)){
         bleAnalogY = 0;
      }
      // Set Analog Pad/Left Thumb Stick X Y Values in Blegamepad
      bleGamepad.setX(bleAnalogY);
      bleGamepad.setY(bleAnalogX);
      // Track State of Analog Sticks X and Y Values
      currentButtonStates[11] = bleAnalogX;
      currentButtonStates[12] = bleAnalogY;
  
      // Decode Triggers
      char trbuf[] = "00";
        //Left Trigger
        ltValue = (dataArrayBit7[6] * 8) + (dataArrayBit6[6] * 4) + (dataArrayBit5[6] * 2) + (dataArrayBit4[6] * 1);
        bleLTrigger = map(ltValue, 0, 15, -32767, 32767);
        //bleLTrigger = map(ltValue, 0, 15, 0, 32767);
        bleGamepad.setLeftTrigger(bleLTrigger);
        //Track State of L trigger
        currentButtonStates[9] = bleLTrigger;
    
        //sprintf(trbuf, "%02i", ltValue);
        //Serial.print(" LT = ");
        //Serial.print(trbuf);

        //Right Trigger
        rtValue = (dataArrayBit7[5] * 8) + (dataArrayBit6[5] * 4) + (dataArrayBit5[5] * 2) + (dataArrayBit4[5] * 1);
        bleLTrigger = map(rtValue, 0, 15, 32767, -32767);
        bleGamepad.setRightTrigger(bleLTrigger);
        //Track State of R trigger
        currentButtonStates[10] = bleLTrigger;
       //sprintf(trbuf, "%02i", rtValue);
       //Serial.print(" RT = ");
       //Serial.print(trbuf);
     
      //Debug
      //unsigned int wMark1 = uxTaskGetStackHighWaterMark(nullptr);
      //printf("Watermark before SendReportA loop is is %u\n", wMark1);
    
    //All Input states gathered Send Update if current and previous states do not match    
    //bleGamepad.sendReport();  
    if (currentButtonStates != previousButtonStates)
        {
            for (byte currentIndex = 0; currentIndex < numOfButtons; currentIndex++)
            {
                previousButtonStates[currentIndex] = currentButtonStates[currentIndex];
            }

            bleGamepad.sendReport();
       }
    //End all Analog Mode data


    } else {
      // Digital Mode - Process Input
      // ============================

      // DPAD
      if (dataArrayBit6[1] != 0 && dataArrayBit7[1] != 0 && dataArrayBit5[1] != 0 && dataArrayBit4[1] != 0){
        bleGamepad.setHat1(DPAD_CENTERED);
        currentButtonStates[1] = 0;
      }
      if (dataArrayBit4[0] == 0 && (dataArrayBit7[0] != 0 || dataArrayBit6[0] != 0)) {
        //Serial.print(" U ");
        bleGamepad.setHat1(DPAD_UP);
        currentButtonStates[1] = 1;
      }
      if (dataArrayBit5[0] == 0 && (dataArrayBit7[0] != 0 || dataArrayBit6[0] != 0)) {
        //Serial.print(" D ");
        bleGamepad.setHat1(DPAD_DOWN);
        currentButtonStates[1] = 2;
      }
      if (dataArrayBit6[0] == 0 && (dataArrayBit4[0] !=0 || dataArrayBit5[0] != 0)) {
        //Serial.print(" L ");
        bleGamepad.setHat1(DPAD_LEFT);
        currentButtonStates[1] = 3;
      }
      if (dataArrayBit7[0] == 0 && (dataArrayBit4[0] !=0 || dataArrayBit5[0] != 0)) {
        //Serial.print(" R ");
        bleGamepad.setHat1(DPAD_RIGHT);
        currentButtonStates[1] = 4;
      }  
      if (dataArrayBit4[0] == 0 && dataArrayBit6[0] == 0) {
        //Serial.print(" U/L ");
        bleGamepad.setHat1(DPAD_UP_LEFT);
        currentButtonStates[1] = 5;
      }
      if (dataArrayBit4[0] == 0 && dataArrayBit7[0] == 0) {
        //Serial.print(" U/R ");
        bleGamepad.setHat1(DPAD_UP_RIGHT);
        currentButtonStates[1] = 6;
      }
      if (dataArrayBit5[0] == 0 && dataArrayBit6[0] == 0) {
        //Serial.print(" D/L ");
        bleGamepad.setHat1(DPAD_DOWN_LEFT);
        currentButtonStates[1] = 7;
      }
      if (dataArrayBit5[0] == 0 && dataArrayBit7[0] == 0) {
        //Serial.print(" D/R ");
        bleGamepad.setHat1(DPAD_DOWN_RIGHT);
        currentButtonStates[1] = 8;
      }
      
      //Buttons
      if (dataArrayBit2[1] == 0) {
        //Serial.print(" A ");
        bleGamepad.press(BUTTON_4);
        currentButtonStates[3] = 1;
      } else {
        bleGamepad.release(BUTTON_4);
        currentButtonStates[3] = 0;
      }
      if (dataArrayBit0[1] == 0) {
        //Serial.print(" B ");
        bleGamepad.press(BUTTON_1);
        currentButtonStates[4] = 1;
      } else {
        bleGamepad.release(BUTTON_1);
        currentButtonStates[4] = 0;
      }
      if (dataArrayBit1[1] == 0) {
        //Serial.print(" C ");
        bleGamepad.press(BUTTON_2);
        currentButtonStates[5] = 1;
      } else {
        bleGamepad.release(BUTTON_2);
        currentButtonStates[5] = 0;
      }
      if (dataArrayBit6[1] == 0) {
        //Serial.print(" X ");
        bleGamepad.press(BUTTON_7);
        currentButtonStates[6] = 1;
      } else {
        bleGamepad.release(BUTTON_7);
        currentButtonStates[6] = 0;
      }
      if (dataArrayBit5[1] == 0) {
        //Serial.print(" Y ");
        bleGamepad.press(BUTTON_5);
        currentButtonStates[7] = 1;
      } else {
        bleGamepad.release(BUTTON_5);
        currentButtonStates[7] = 0;
      }
      if (dataArrayBit4[1] == 0) {
        //Serial.print(" Z ");
        bleGamepad.press(BUTTON_8);
        currentButtonStates[8] = 1;
      } else {
        bleGamepad.release(BUTTON_8);
        currentButtonStates[8] = 0;
      }
      if (dataArrayBit3[2] == 0) {
        //Serial.print(" LT ");
        bleGamepad.press(BUTTON_11);
        currentButtonStates[9] = 1;
      } else {
        bleGamepad.release(BUTTON_11);
        currentButtonStates[9] = 0;
      }
      if (dataArrayBit7[1] == 0) {
        //Serial.print(" RT ");
        bleGamepad.press(BUTTON_13);
        currentButtonStates[10] = 1;
      } else {
        bleGamepad.release(BUTTON_13);
        currentButtonStates[10] = 0;
      }
      if (dataArrayBit3[1] == 0) {
        //Serial.println(" Start ");
        bleGamepad.press(BUTTON_12);
        currentButtonStates[2] = 1;
      } else {
        ////Serial.println(" ");
        bleGamepad.release(BUTTON_12);
        currentButtonStates[2] = 0;
      }
      // Latency Test Pin
      // Intended for BlueRetro latency test pad to measure BLEGamepad update speeds without modifying the Controller
      if (digitalRead(latPin) == LOW) {
        Serial.print("Latency Test Press");
        bleGamepad.press(BUTTON_4);
        currentButtonStates[2] = 1;
            } else {
        bleGamepad.release(BUTTON_4);
        currentButtonStates[2] = 0;
        }
         
      // unsigned int wMark2 = uxTaskGetStackHighWaterMark(nullptr);
      // printf("Watermark - SendReport B loop %u\n", wMark2); 

    // All Input states gathered Send Update if state has changed
    if (currentButtonStates != previousButtonStates){
          for (byte currentIndex = 0; currentIndex < numOfButtons; currentIndex++)
            {
            previousButtonStates[currentIndex] = currentButtonStates[currentIndex];
            }
          bleGamepad.sendReport();
       } 
    }
  // End Digital Mode - Process Input
  // ================================


  // Invalid Data or no connection error state
  // =========================================
  // Communication or wiring issue
  } else {
      for (int printByteCounter = 0; printByteCounter < modeCounter + 1; printByteCounter++) {
        Serial.println("Error: Gamepad may not be paired OR Pinout/Timing Incorrect");
        //Uncomment lines for additional debug output
        //Serial.print("  -- Byte ");
        //Serial.print(printByteCounter);
        //Serial.print(" - ");
        //Serial.print(dataArrayBit0[printByteCounter]);
        //Serial.print(dataArrayBit1[printByteCounter]);
        //Serial.print(dataArrayBit2[printByteCounter]);
        //Serial.print(dataArrayBit3[printByteCounter]);
        //Serial.print(dataArrayBit4[printByteCounter]);
        //Serial.print(dataArrayBit5[printByteCounter]);
        //Serial.print(dataArrayBit6[printByteCounter]);
        //Serial.print(dataArrayBit7[printByteCounter]);
      }
      // End Invalid Data
      //=================
    }
    // End Polling 
    // ===========
    
  // Reset variables ready for data read.
  byteCounter = 0;
  nibble0Read = 0;

  // Setting Current Time for data read
  timeStamp = millis();      
  }


  // Check to see if the Controller is ready to send data and if we have read any data yet
  if (TLACK == HIGH and nibble0Read == 0) {  
      digitalWrite(THPin, LOW);
      delayMicroseconds(4);   // I don't think I need this
      digitalWrite(TRPin, LOW);
      delayMicroseconds(4);
      //delayMicroseconds(1);
  }


  // If the 1st nibble is being sent and we havn't read the 1st nibble yet get the data from the pins
  if (TLACK == LOW and nibble0Read == 0) {
      // Sets ready to read the 1st nibble of data
      digitalWrite(TRPin, HIGH);
      delayMicroseconds(8); //For ESP32 with lower setting get random button press

      // Read the Data for the 1st Nibble
      dataArrayBit0[byteCounter] = digitalRead(dataPinD0);
      dataArrayBit1[byteCounter] = digitalRead(dataPinD1);
      dataArrayBit2[byteCounter] = digitalRead(dataPinD2);
      dataArrayBit3[byteCounter] = digitalRead(dataPinD3);

      if (dataArrayBit0[byteCounter] > 1) {
        dataArrayBit0[byteCounter] = 0;
      }
      if (dataArrayBit1[byteCounter] > 1) {
        dataArrayBit1[byteCounter] = 0;
      }
      if (dataArrayBit2[byteCounter] > 1) {
        dataArrayBit2[byteCounter] = 0;
      }
      if (dataArrayBit3[byteCounter] > 1) {
        dataArrayBit3[byteCounter] = 0;
      }

      // 1st Nibble has been read
      nibble0Read = 1;   

      // Check if we are reading data for Analogue mode or Digital Mode
      // The mode defines the number of Bytes we need to read
      if (byteCounter == 0) {
        if ((dataArrayBit0[0] == 1 and dataArrayBit1[0] == 0 and dataArrayBit2[0] == 0 and dataArrayBit3[0] == 0) or (dataArrayBit0[0] == 0 and dataArrayBit1[0] == 1 and dataArrayBit2[0] == 1 and dataArrayBit3[0] == 0) or (dataArrayBit0[0] == 1 and dataArrayBit1[0] == 1 and dataArrayBit2[0] == 1 and dataArrayBit3[0] == 0) )  {
          modeCounter = AnalogMode;
        } else {
          modeCounter = DigitalMode;
        }
      }
    }

  // Check if the 2nd Nibble is being sent and if we have already read the 1st Nibble
  if (TLACK == HIGH and nibble0Read == 1) {
      // Sets ready to read the 2nd Nibble
      digitalWrite(TRPin, LOW);
      delayMicroseconds(8);
      // Read the data for the 2nd Nibble
      dataArrayBit4[byteCounter] = digitalRead(dataPinD0);
      dataArrayBit5[byteCounter] = digitalRead(dataPinD1);
      dataArrayBit6[byteCounter] = digitalRead(dataPinD2);
      dataArrayBit7[byteCounter] = digitalRead(dataPinD3);
      if (dataArrayBit4[byteCounter] > 1) {
        dataArrayBit4[byteCounter] = 0;
      }
      if (dataArrayBit5[byteCounter] > 1) {
        dataArrayBit5[byteCounter] = 0;
      }
      if (dataArrayBit6[byteCounter] > 1) {
        dataArrayBit6[byteCounter] = 0;
      }
      if (dataArrayBit7[byteCounter] > 1) {
        dataArrayBit7[byteCounter] = 0;
      }

      // Reset the 1st Nibble read variable, ready for the next Byte of data
      nibble0Read = 0;
      
      // Increase the Byte counter by 1 so we are ready to read the next Byte of data
      byteCounter++;
  }

  // Time Stamp Routines to check if data has paused.
  currentMillis = millis();

  if (currentMillis - timeStamp >= interval) {
    // Resetting All Variables
    timeStamp = millis();
    digitalWrite(THPin, HIGH);
    digitalWrite(TRPin, HIGH);
    byteCounter = 0;
    nibble0Read = 0;
    Serial.println("Resetting");
    }
}
//============================= 
// End Controller related items

// Pin anything that isn't the controller or bluetooth to core 0
//==============================================================

void Task1code( void * pvParameters ){
  //Serial.print("OLED Update ESP Core ");
  //Serial.println(xPortGetCoreID());

  for(;;){
    //Serial.print("timeStamp val" );
    if (timeStamp == 0){
      bootLogo();
      timeStamp = 1;
    }
    oled_Updates();
    getBatStat();
  }
}
// End Core Pinning 
//=================


// Controller and Bluetooth Jobs here
void Task2code( void * pvParameters ){
  //Serial.print("Task2code running on core ");
  //Serial.println(xPortGetCoreID());

  for(;;){
    //debug
    //  unsigned int wMark3 = uxTaskGetStackHighWaterMark(nullptr);
    //   printf("Watermark before FOR Loop is %u\n", wMark3);

    pollController();
    //debug
    //  unsigned int wMark = uxTaskGetStackHighWaterMark(nullptr);
    //  printf("Watermark after FOR Loop is %u\n", wMark);
  }
}


// This is the main loop due to threading in arduino + core tasking in RTOS
//==========================================================================
void setup() {
  // Check ESP Core
  //Serial.print("Setup Core");
  //Serial.println(xPortGetCoreID());

  
  // Start BLE Gamepad  
    BleGamepadConfiguration bleGamepadConfig;
    bleGamepadConfig.setControllerType(CONTROLLER_TYPE_GAMEPAD); // CONTROLLER_TYPE_JOYSTICK, CONTROLLER_TYPE_GAMEPAD (DEFAULT), CONTROLLER_TYPE_MULTI_AXIS
    bleGamepadConfig.setButtonCount(numOfButtons);
    bleGamepadConfig.setWhichAxes(enableX, enableY, enableZ, enableRX, enableRY, enableRZ, enableSlider1, enableSlider2);      // Can also be done per-axis individually. All are true by default
    bleGamepadConfig.setHatSwitchCount(numOfHatSwitches);                                                                      // 1 by default
    bleGamepadConfig.setAxesMin(0x8001); // -32767 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal
    bleGamepadConfig.setAxesMax(0x7FFF); // 32767 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal 
    // Other Available Special Button Options
    // bleGamepadConfig.setIncludeStart(true);
    // bleGamepadConfig.setIncludeSelect(true);
    // bleGamepadConfig.setIncludeMenu(true);
    // bleGamepadConfig.setIncludeHome(true);
    // bleGamepadConfig.setIncludeBack(true);
    // bleGamepadConfig.setIncludeVolumeInc(true);
    // bleGamepadConfig.setIncludeVolumeDec(true);
    // bleGamepadConfig.setIncludeVolumeMute(true);
    // Use or disable BLEGampead AutoReporting
    // AutoReport can impact latency negatively with high speed of updates
    bleGamepadConfig.setAutoReport(false); // This is true by default  
    bleGamepad.begin(&bleGamepadConfig);

    
  // Setting input and output pins for Comms with Controller
  // Use Heavy and Lightwing pinout settings instead of changing this
  //==========================// Pin 1 = +5V
  pinMode(dataPinD1, INPUT);  // Pin 2 = Data D1
  pinMode(dataPinD0, INPUT);  // Pin 3 = Data D0
  pinMode(THPin, OUTPUT);     // Pin 4 = TH Select
  pinMode(TRPin, OUTPUT);     // Pin 5 = TR Request
  pinMode(TLPin, INPUT);      // Pin 6 = TL Response
  pinMode(dataPinD3, INPUT);  // Pin 7 = Data D3
  pinMode(dataPinD2, INPUT);  // Pin 8 = Data D2
  //==========================// Pin 9 = GND

  // Set pin state to begin reading controller state
  digitalWrite(THPin, HIGH);
  digitalWrite(TRPin, HIGH);

  // Define Latency test pin
  pinMode(latPin,INPUT_PULLUP );


  // Setup Serial comms for Serial Monitor
  Serial.begin(115200);
  Serial.print("Device Booting....");

  // Creates Tasks for Core 1
  xTaskCreatePinnedToCore(
  Task1code,   /* Task function. */
  "Task1",     /* name of task. */
  5000,       /* Stack size of task */                    //originally 10k
  NULL,        /* parameter of the task */
  1,           /* priority of the task */
  &Task1,      /* Task handle to keep track of created task */
  1);          /* pin task to core 0 */

  //Creates Tasks for Core 0
  xTaskCreatePinnedToCore(
  Task2code,   /* Task function. */
  "Task2",     /* name of task. */
  5000,       /* Stack size of task */
  NULL,        /* parameter of the task */
  1,           /* priority of the task */
  &Task2,      /* Task handle to keep track of created task */
  0);          /* pin task to core 1 */

}


// Loop will always run on Core 1
void loop(){
  // Putting stuff here will break some automatic aspects of RTOS task cleanup
  // All tasks/jobs should be performed from setup loop
  }

//Currently unused setup for Motion Controls
// All this code is trashed and intermixed (sorry) but the idea is
// initializeSensor -> setupMPU -> GetTiltMpu
void initializeSensor(){
  // Perfrom full reset as per MPU-6000/MPU-6050 Register Map and Descriptions, Section 4.28, pages 40 to 41.
  // performing full device reset, disables temperature sensor, disables SLEEP mode
  Wire.beginTransmission(0x68);  // Device address.
  Wire.write(0x6B);              // PWR_MGMT_1 register.
  Wire.write(0b10001000);        // DEVICE_RESET, TEMP_DIS.
  Wire.endTransmission();
  delay(100);                    // Wait for reset to complete.

  Wire.beginTransmission(0x68);  // Device address.
  Wire.write(0x68);              // SIGNAL_PATH_RESET register.
  Wire.write(0b00000111);        // GYRO_RESET, ACCEL_RESET, TEMP_RESET.
  Wire.endTransmission();
  delay(100);                    // Wait for reset to complete.

  // Disable SLEEP mode because the reset re-enables it. Section 3, PWR_MGMT_1 register, page 8.
  Wire.beginTransmission(MPU_addr);   // Device address.
  Wire.write(0x6B);                     // PWR_MGMT_1 register.
  Wire.write(0b00001000);               // SLEEP = 0, TEMP_DIS = 1.
  Wire.endTransmission();
}
// =============
// End Start MPU

void setupMPU() {
  //Setup Accelerometer
  //Wire.begin();

  //Configure MPU Connectivity
  //Wire.beginTransmission(MPU_addr);
  //Wire.write(0x68);
  //Wire.write(0x00);                  // Make reset - place a 0 into the 6B register
  //Wire.endTransmission(true); 
  //Wire.write(0);
  //Wire.endTransmission(true);

  
  //Adafruit_MPU6050 mpu;
  //if (!Wire.begin()) {
  //  Serial.println("Failed to find MPU6050 chip");
  //  while (1) {
  //    delay(10);
  // }
  //}
  //Serial.println("MPU6050 Found!");
  //Setup Accelerometer End
}



void getTiltMPU() {
  //sensors_event_t a, g, temp;
  //mpu.getEvent(&a, &g, &temp);
//Wire.begin();

//Configure MPU Connectivity
//Wire.beginTransmission(MPU_addr);
//Wire.write(0x68);
//Wire.write(0);
//Wire.endTransmission(true);

//Serial.begin(115200);
//void loop(){

//Get Data from MPU
//Wire.beginTransmission(MPU_addr);
//Wire.write(0x3B);
//Wire.endTransmission(false);
//Wire.requestFrom(MPU_addr,14,true);

//Store MPU Values
//AcX=Wire.read()<<8|Wire.read();
//AcY=Wire.read()<<8|Wire.read();
//AcZ=Wire.read()<<8|Wire.read();

Serial.print("MPU X Y Z");
//Serial.print(a.acceleration.x, 1);
//Serial.print(a.acceleration.y, 1);
//Serial.print(a.acceleration.z, 1);
Serial.print("MPU GYRO");
//Serial.print(g.gyro.x, 1);
//Serial.print(g.gyro.y, 1);
//Serial.print(g.gyro.z, 1);

//Convert stored values from Rads to Degrees
//int xAng = map(AcX,minVal,maxVal,-90,90);
//int yAng = map(AcY,minVal,maxVal,-90,90);
//int zAng = map(AcZ,minVal,maxVal,-90,90);
//x= RAD_TO_DEG * (atan2(-yAng, -zAng)+PI);
//y= RAD_TO_DEG * (atan2(-xAng, -zAng)+PI);
//z= RAD_TO_DEG * (atan2(-yAng, -xAng)+PI);

//Print current angles
//Serial.print("AngleX  ");
//Serial.println(x);
//Serial.print("AngleY  ");
//Serial.println(y);
//display.clearDisplay();
//display.setTextSize(1);             // Normal 1:1 pixel scale
//display.setTextColor(SSD1306_WHITE, BLACK);        // Draw white text
//display.setCursor(0,10);
//display.println("Tilt: On");
//display.setCursor(0,20);             // r
//String derp1 = "   X Axis: " + String(x);
//String derp2 = "   Y Axis: " + String(y);
//display.clearDisplay();
//display.println(derp1);
//display.println(F("X Axis " + String(x)));
//display.setCursor(0,30);
//display.println(derp2);
//display.println(F("y Axis " + String(y)));
//display.display();
//

//Limit Degrees of angle based on comfort prior to converting to BLEGamepad Vals
//Limit X
if (x < 160){
  x=160;
}
if (x > 200){
  x=200;
}
//Limit Y
if (y < 160){
  y=160;
}
if (y > 200){
  y=200;
}

//Define DeadZones 
//Will require calibration based on your install
//
//Create X deadzone
if ((x >= 170) && (x <= 190)){
  x=180;
}
//
//Create Y deadzone
if ((y >= 170) && (y <= 190)){
  y=180;
}

//Map Values to Gamepad Ranges
xmap = map (x, 160, 200, 0, 32767);
ymap = map (y, 160, 200, 0, 32767);
//Print Mapped values
//Serial.print("Xmap BleG: ");
//Serial.println(xmap);
//Serial.print("Ymap BleG: ");
//Serial.println(ymap);

Serial.println("-----------------------------------------");

}
