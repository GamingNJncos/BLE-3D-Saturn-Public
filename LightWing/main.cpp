// Add BLE Gamepad
#include <BleGamepad.h>
// Add Sleep
//#include "esp_sleep.h"

/*
.____    .__       .__     __   __      __.__                
|    |   |__| ____ |  |___/  |_/  \    /  \__| ____    ____  
|    |   |  |/ __ \|  |  \   __\   \/\/   /  |/    \  / __ \ 
|    |___|  / /_/  |   Y  \  |  \        /|  |   |  \/ /_/  >
|_______ \  \___  /|___|  /__|   \__/\  / |  |___|  /\___  / 
    \_____\/_____/      \/            \/___\/_____\/______/
	
    Brought to you by GaminNJncos, Humble Bazooka & Hexfreq 	
    All bugs, poor code choices, and existing PCB designs courtesy of [at]GamingNJncos
*/

/* About this fork
   Goal of this fork is to focus on core functionality with the lowest latency
   This fork has no support for the Heavywing features including rumble, tilt, or LCD
   
   As of writing there is no deep sleep support which means you have to unplug the battery on the feather (esp32)
   to turn it off, or install a switch on your battery. There is an additional caveat that for charging, the 
   feather must be turned on. This is a result of the feather and it's charging circuit and not intended as a long
   term solution
*/
   
// BLEGamePad Settings
// ===================
// Values are Advertised Name, Manufacturer, Battery level
// There is a char limit on these
BleGamepad bleGamepad("LightWing", "GnJ & Humble Bazooka", 100);
// Purposely set high to permit ease of future additional button combinationsfor to send Meta, home button, etc
#define numOfButtons 16
#define numOfHatSwitches 1        // DPAD (not Analog pad)
#define enableX true              // Analog Pad
#define enableY true              // Analog Pad
#define enableAccelerator true    // RT
#define enableBrake true          // LT
// Additional button options currently disabled
#define enableZ false
#define enableRZ false
#define enableSlider1 false
#define enableSlider2 false
#define enableRudder false
#define enableThrottle false
#define enableRX false
#define enableRY false
#define enableSteering false
// End BLEGamePad Settings

// Button State Tracking
// This nets a significant advantage in performance vs not tracking states
// BLE expects ACK per packet so flooding "not pressed" or identical data can add extreme delays
//
byte previousButtonStates[numOfButtons];
byte currentButtonStates[numOfButtons];
//  Button State Array Map
// The below numeric values are the data returned from poll controller
// Some are binary 0 or 1, while others are a range as read from the 3d controller protocol
//
//  --------------------------------------------------------------------
//  MODE,   DPAD,   START, A, B, C, X, Y, Z,     L, R,       Analog PAD
//
//  --------------------------------------------------------------------
//  Note Left and Right stick in digital mode will return Binary [0-1] vs [0-15] value in analog mode
//  In Digital mode no values are returned from the Analog Stick

//  BLE Gamepad index Mapping
//  These are the values for each individual button as they relate to mappings in BLE Gamepad library
//  This table might be out of date?

//  PAD         BLE Index
//  ---------------------

//  Mode        0
//  DPAD        1
//  START       2
//  A           3
//  B           4
//  C           5
//  X           6
//  Y           7
//  Z           8
//  LT          9
//  RT          10


// Pinmappings for your PCB version
// ================================
// It is manditory you select the correct pinout for your board type
// Failure to do so will cause Segata to light your 3d controller on fire and beat your loved ones

// Feather LightWing to Controller Pin Mapping
// ===========================================
// PCB: https://oshpark.com/shared_projects/7RqwDWJT
//
int dataPinD0 = 12; //  Arthrimus Pin 3 = Data D0
int dataPinD1 = 13; //  Arthrimus Pin 2 = Data D1
int dataPinD2 = 14; //  Arthrimus Pin 8 = Data D2
int dataPinD3 = 32; //  Arthrimus Pin 7 = Data D3
int THPin = 27;     // Arthrimus Pin 4 = TH Select
int TRPin = 33;     // Arthrimus Pin 5 = TR Request
int TLPin = 15;     //    Arthrimus Pin 6 = TL Response
int latPin = 4;     //  Latency testing pin
// End Feather HeavyWing to Controller Pin Mapping
// End all Pin Mappings

// BleGamepad Variables
// ====================
int16_t bleAnalogX; // Analog pad X Axis Output
int16_t bleAnalogY; // Analog pad Y Axis Output
// DeadZone defined as smallest value to exceed before sending in blegamepad report function
int16_t bleDeadZone = 400; // Set Deadzone via minimum number
// Analog Triggers
int16_t bleRTrigger; // Right Trigger Analog Output
int16_t bleLTrigger; // Left Trigger Analog Output
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
int nibble0Read = 0; // Confirm 1st Nibble has been read
int byteCounter = 0; // Current Byte Tracker
int modeCounter = 4; // Reference for which mode we are currently in Analogue or Digital
int DecodeData = 1;  // Controller Successfully Decoded Data Variable
int curMode = 0;     // This stores the Controller Mode to pass between ESP cores
// Assorted timing oriented variables
unsigned long timeStamp = 0;            // Store time of last update
unsigned long bleTimer = 0;
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

// Start CPU variables
// ===================
// These variables are used for CPU Speed settings and tasks
uint32_t Freq = 0; //Used for getCPU debug
//CPU MHz //
int CpuMhz = 40; //Change to set CPU Freq 
// End CPU variables

//int firstBoot = 0; //First Boot check
//-----------------Deep Sleep test---------------
//int firstBoot = 0; //First Boot check  //Disabled to test firstboot check in RTC
// This doesn't need to currently be stored in RTC but is helpful to call troubleshooting routines only on power up
RTC_DATA_ATTR int firstBoot = 0; //First Boot check

// Allocate the interrupt.
///esp_err_t err = esp_intr_alloc(ETS_INTR_DEFAULT, my_interrupt_handler, NULL, 0);
//if (err != ESP_OK) {
  // Handle the error.
//}

//-----------------Deep Sleep test---------------

// End CPU variables
// ===================


// RTC deep sleep time
//
// This variable defines the length of time to sleep prior to checking for button press
// Intended design is that when in deep sleep a user must hold down a button on the controller
// For longer than the configured deep sleep to wake back up
//
// Current example is 10 seconds
#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  30        /* Time ESP32 wi go to sleep (in seconds) */


// Enter Deep Sleep
void enterDeepSleep()
{
  Serial.println("GOING TO DEEP SLEEP ZZZzz...");
  // Disable the Bluetooth receiver
  //esp_bt_controller_disable();
  // Set the Bluetooth controller to power-off mode.
  //esp_bt_controller_set_power_mode(ESP_BT_POWER_MODE_OFF, ESP_PD_OPTION_OFF)

  // Set the Bluetooth host stack to power-off mode.
   btStop();

  // Delete all of the FreeRTOS tasks
  //vTaskDelete(NULL); Needed when core pinning is used with RTOS


  delay(10);
  // Go to deep sleep
  esp_deep_sleep_start();
}
// End CPU variables
// ===================


// Controller related items
// ========================
void pollController()
{
  //bleGamepad.setRightThumb(0,0);
  // Check ESP Core Controller Polling Occurs on
  // Serial.print("Controller Polling Core ");
  // Serial.println(xPortGetCoreID());

  // Start The Data Read
  // Is the Controller ready to send the data?
  TLACK = digitalRead(TLPin);

  // Have we read all the bytes needed for each mode?
  if (byteCounter > modeCounter)
  {
    // Set Not ready for Data Coms
    digitalWrite(THPin, HIGH);
    digitalWrite(TRPin, HIGH);

    // Are we using Analogue Mode or Digital?
    if (modeCounter == AnalogMode)
    {
      curMode = 0;
      currentButtonStates[0] = 0;
    }
    else
    {
      curMode = 1;
      currentButtonStates[0] = 1;
    }

    // Decoded Controller Data Successfully
    // Proceed based on Analog or Digital Mode
    //
    // Note bleGamepad.isConnected()isconnected check should always happen before gamepadsend 

    // This avoids a bug in NIMBLE where blegamepad could under some circumstances attempt button send prior to connecting and cause abort

    if (DecodeData == 1 && bleGamepad.isConnected())

    {
      // Analog
      if (modeCounter == AnalogMode)
      {
        
        // Analog Mode - Process Input
        // Begin all Analog Mode data processing

        // DPAD
        if (dataArrayBit6[1] != 0 && dataArrayBit7[1] != 0 && dataArrayBit5[1] != 0 && dataArrayBit4[1] != 0)
        {
          bleGamepad.setHat1(DPAD_CENTERED);
          currentButtonStates[1] = 0;
        }
        if (dataArrayBit4[1] == 0 && (dataArrayBit7[1] != 0 && dataArrayBit6[1] != 0))
        {
          // Serial.print(" U");
          bleGamepad.setHat1(DPAD_UP);
          currentButtonStates[1] = 1;
        }
        if (dataArrayBit5[1] == 0 && (dataArrayBit7[1] != 0 && dataArrayBit6[1] != 0))
        {
          // Serial.print(" D");
          bleGamepad.setHat1(DPAD_DOWN);
          currentButtonStates[1] = 2;
        }
        if (dataArrayBit6[1] == 0 && (dataArrayBit4[1] != 0 && dataArrayBit5[1] != 0))
        {
          // Serial.print(" L");
          bleGamepad.setHat1(DPAD_LEFT);
          currentButtonStates[1] = 3;
        }
        if (dataArrayBit7[1] == 0 && (dataArrayBit4[1] != 0 && dataArrayBit5[1] != 0))
        {
          // Serial.print(" R");
          bleGamepad.setHat1(DPAD_RIGHT);
          currentButtonStates[1] = 4;
        }
        if (dataArrayBit4[1] == 0 && dataArrayBit6[1] == 0)
        {
          // Serial.print(" U/L");
          bleGamepad.setHat1(DPAD_UP_LEFT);
          currentButtonStates[1] = 5;
        }
        if (dataArrayBit4[1] == 0 && dataArrayBit7[1] == 0)
        {
          // Serial.print(" U/R");
          bleGamepad.setHat1(DPAD_UP_RIGHT);
          currentButtonStates[1] = 6;
        }
        if (dataArrayBit5[1] == 0 && dataArrayBit6[1] == 0)
        {
          // Serial.print(" D/L");
          bleGamepad.setHat1(DPAD_DOWN_LEFT);
          currentButtonStates[1] = 7;
        }
        if (dataArrayBit5[1] == 0 && dataArrayBit7[1] == 0)
        {
          // Serial.print(" D/R");
          bleGamepad.setHat1(DPAD_DOWN_RIGHT);
          currentButtonStates[1] = 8;
        }
        // End DPAD

        // Buttons
        if (dataArrayBit3[2] == 0)
        {
          // Serial.print(" Start ");
          bleGamepad.press(BUTTON_12);
          currentButtonStates[2] = 1;
        }
        else
        {
          bleGamepad.release(BUTTON_12);
          currentButtonStates[2] = 0;
        }
        if (dataArrayBit2[2] == 0)
        {
          // Serial.print(" A");
          bleGamepad.press(BUTTON_4);
          currentButtonStates[3] = 1;
        }
        else
        {
          // Serial.print(" ");
          bleGamepad.release(BUTTON_4);
          currentButtonStates[3] = 0;
        }
        if (dataArrayBit0[2] == 0)
        {
          // Serial.print(" B");
          bleGamepad.press(BUTTON_1);
          currentButtonStates[4] = 1;
        }
        else
        {
          // Serial.print(" ");
          bleGamepad.release(BUTTON_1);
          currentButtonStates[4] = 0;
        }
        if (dataArrayBit1[2] == 0)
        {
          // Serial.print(" C");
          bleGamepad.press(BUTTON_2);
          currentButtonStates[5] = 1;
        }
        else
        {
          // Serial.print(" ");
          bleGamepad.release(BUTTON_2);
          currentButtonStates[5] = 0;
        }
        if (dataArrayBit6[2] == 0)
        {
          // Serial.print(" X");
          bleGamepad.press(BUTTON_7);
          currentButtonStates[6] = 1;
        }
        else
        {
          // Serial.print(" ");
          bleGamepad.release(BUTTON_7);
          currentButtonStates[6] = 0;
        }
        if (dataArrayBit5[2] == 0)
        {
          // Serial.print(" Y");
          bleGamepad.press(BUTTON_5);
          currentButtonStates[7] = 1;
        }
        else
        {
          // Serial.print(" ");
          bleGamepad.release(BUTTON_5);
          currentButtonStates[7] = 0;
        }
        if (dataArrayBit4[2] == 0)
        {
          // Serial.print(" Z");
          bleGamepad.press(BUTTON_8);
          currentButtonStates[8] = 1;
        }
        else
        {
          // Serial.print(" ");
          bleGamepad.release(BUTTON_8);
          currentButtonStates[8] = 0;
        }
        // These can send the trigger presses as a button vs an axis

        /*
        if (dataArrayBit3[3] == 0) {
          Serial.print(" LTD ");
          bleGamepad.press(BUTTON_9);
          currentButtonStates[13] = 1; //This cant overlap with LTA state index
        } else {
          bleGamepad.release(BUTTON_9);
          currentButtonStates[13] = 0; //This cant overlap with LTA state index
        }


        // if (dataArrayBit7[2] == 0) {
        if (dataArrayBit7[2] == 0) {
          Serial.print(" RTD ");
          bleGamepad.press(BUTTON_13);
          currentButtonStates[14] = 1; //This cant overlap with RTA state index
        } else {
          bleGamepad.release(BUTTON_13);
          currentButtonStates[14] = 0; //This cant overlap with RTA state index
        }

        */
    
        
        // Latency Test Pin
        // Intended for BlueRetro latency button
        // Can be used for other additional buttons
        //
        // Note this sends "Start" but can be adjusted to press any other button

        /*
        if (digitalRead(latPin) == LOW)
        {
          // Serial.print("LAT");
          bleGamepad.press(BUTTON_3);
          currentButtonStates[3] = 1;
        }
        else
        {
          bleGamepad.release(BUTTON_3);
          currentButtonStates[3] = 0;
        }

        */
        // Up / Down
        if (dataArrayBit7[4] == 1)
        {
          upDownValue = dataArrayBit0[5] + (dataArrayBit1[5] << 1) + (dataArrayBit2[5] << 2) + (dataArrayBit3[5] << 3) + (dataArrayBit4[4] << 4) + (dataArrayBit5[4] << 5) + (dataArrayBit6[4] << 6);


          bleAnalogX = map(upDownValue, 0, 127, 16384, 32767);


        }
        else
        {
          upDownValue = !dataArrayBit0[5] + (!dataArrayBit1[5] << 1) + (!dataArrayBit2[5] << 2) + (!dataArrayBit3[5] << 3) + (!dataArrayBit4[4] << 4) + (!dataArrayBit5[4] << 5) + (!dataArrayBit6[4] << 6);


          bleAnalogX = map(upDownValue, 0, 127, 16383, 1);


        }
        // Deadzone fix - X Axis
        // Value is checked based on map output
         if ((bleAnalogX < 0 && bleAnalogX > -600)||(bleAnalogX < 0 && bleAnalogX > -600)){
           bleAnalogX = 0;
        }

        // Analog Debugs
        // Serial.print(" bleAnalogX ");
        // Serial.println(bleAnalogX);
        //  Format Value to 3 Chars long
        // char buf[] = "000";
        // sprintf(buf, "%03i", upDownValue);

        // Left / Right
        if (dataArrayBit7[3] == 1)
        {
          // Serial.print(" R = ");
          leftRightValue = dataArrayBit0[4] + (dataArrayBit1[4] << 1) + (dataArrayBit2[4] << 2) + (dataArrayBit3[4] << 3) + (dataArrayBit4[3] << 4) + (dataArrayBit5[3] << 5) + (dataArrayBit6[3] << 6);


          bleAnalogY = map(leftRightValue, 0, 127, 16384, 32767);


        }
        else
        {
          leftRightValue = !dataArrayBit0[4] + (!dataArrayBit1[4] << 1) + (!dataArrayBit2[4] << 2) + (!dataArrayBit3[4] << 3) + (!dataArrayBit4[3] << 4) + (!dataArrayBit5[3] << 5) + (!dataArrayBit6[3] << 6);


          bleAnalogY = map(leftRightValue, 0, 127, 16383, 1);


        }

        // Deadzone fix - Y Axis
        // Value is checked based on map output
         if ((bleAnalogY > 0 && bleAnalogY < 600)||(bleAnalogY < 0 && bleAnalogY > -600)){
           bleAnalogY = 0;
           }
        // Set Analog Pad/Left Thumb Stick X Y Values in Blegamepad
        // This is an easy place to flip the X and Y results if backwards for your use case
        bleGamepad.setX(bleAnalogY);
        bleGamepad.setY(bleAnalogX);
        // Track State of Analog Sticks X and Y Values
        currentButtonStates[11] = bleAnalogX;
        currentButtonStates[12] = bleAnalogY;
        // unused at this time
        // currentButtonStates[13] = 0;
        // currentButtonStates[14] = 0;

        // Decode Triggers
        char trbuf[] = "00";
        // Left Trigger
        ltValue = (dataArrayBit7[6] * 8) + (dataArrayBit6[6] * 4) + (dataArrayBit5[6] * 2) + (dataArrayBit4[6] * 1);  
        bleLTrigger = map(ltValue, 0, 15, 0, 32767); 
		// Example center fixes if axis issue occur
        //if (bleLTrigger == 0){bleLTrigger = 16384;} //center fix
        //bleLTrigger = map(ltValue, 0, 15, 0, 255);
        //if (bleLTrigger == 0){bleLTrigger = 128;}
        // bleGamepad.setLeftTrigger(bleLTrigger);

        bleGamepad.setBrake(bleLTrigger);
        // Track State of L trigger
        currentButtonStates[9] = bleLTrigger;
		
        // Debug contents of LTA
        //if (bleLTrigger != 128){Serial.print(" LTA = ");Serial.println(bleLTrigger);}
        
        //sprintf(trbuf, "%02i", ltValue);
        // Serial.print(" LT = ");
        // Serial.print(trbuf);
        // Serial.println(bleLTrigger);
        
        
        // Right Trigger
        rtValue = (dataArrayBit7[5] * 8) + (dataArrayBit6[5] * 4) + (dataArrayBit5[5] * 2) + (dataArrayBit4[5] * 1);

        bleRTrigger = map(rtValue, 0, 15, 0, 32767);    //Map if 0-32K is range
		// Example center fixes if axis issue occur


        //if (bleRTrigger == 0){bleRTrigger = 16384;}     //<--Center Fix for 0-32k Range
        //bleRTrigger = map(rtValue, 0, 15, 0, 255);    //<-- Map if 0-255 Range
        //if (bleRTrigger == 0){bleRTrigger = 128;}     // <--Center Fix for 0-255 Range 
        

        bleGamepad.setAccelerator(bleRTrigger);
        // Track State of R trigger
        currentButtonStates[10] = bleRTrigger;
        // sprintf(trbuf, "%02i", rtValue);
         
        //Debug contents of LTA
        //if (bleRTrigger != 128){Serial.print(" RTA = ");Serial.println(bleRTrigger);}

        // End all Analog Mode data
      }
      else
      {
        // Digital Mode - Process Input
        // ============================

        // DPAD
        if (dataArrayBit6[0] != 0 && dataArrayBit7[0] != 0 && dataArrayBit5[0] != 0 && dataArrayBit4[0] != 0)
        {
          bleGamepad.setHat1(DPAD_CENTERED);
          currentButtonStates[1] = 0;
        }
        if (dataArrayBit4[0] == 0 && (dataArrayBit7[0] != 0 || dataArrayBit6[0] != 0))
        {
          // Serial.print(" U ");
          bleGamepad.setHat1(DPAD_UP);
          currentButtonStates[1] = 1;
        }
        if (dataArrayBit5[0] == 0 && (dataArrayBit7[0] != 0 || dataArrayBit6[0] != 0))
        {
          // Serial.print(" D ");
          bleGamepad.setHat1(DPAD_DOWN);
          currentButtonStates[1] = 2;
        }
        if (dataArrayBit6[0] == 0 && (dataArrayBit4[0] != 0 || dataArrayBit5[0] != 0))
        {
          // Serial.print(" L ");
          bleGamepad.setHat1(DPAD_LEFT);
          currentButtonStates[1] = 3;
        }
        if (dataArrayBit7[0] == 0 && (dataArrayBit4[0] != 0 || dataArrayBit5[0] != 0))
        {
          // Serial.print(" R ");
          bleGamepad.setHat1(DPAD_RIGHT);
          currentButtonStates[1] = 4;
        }
        if (dataArrayBit4[0] == 0 && dataArrayBit6[0] == 0)
        {
          // Serial.print(" U/L ");
          bleGamepad.setHat1(DPAD_UP_LEFT);
          currentButtonStates[1] = 5;
        }
        if (dataArrayBit4[0] == 0 && dataArrayBit7[0] == 0)
        {
          // Serial.print(" U/R ");
          bleGamepad.setHat1(DPAD_UP_RIGHT);
          currentButtonStates[1] = 6;
        }
        if (dataArrayBit5[0] == 0 && dataArrayBit6[0] == 0)
        {
          // Serial.print(" D/L ");
          bleGamepad.setHat1(DPAD_DOWN_LEFT);
          currentButtonStates[1] = 7;
        }
        if (dataArrayBit5[0] == 0 && dataArrayBit7[0] == 0)
        {
          // Serial.print(" D/R ");
          bleGamepad.setHat1(DPAD_DOWN_RIGHT);
          currentButtonStates[1] = 8;
        }

        // Buttons
        
        //------- Start Button -------//

        if (dataArrayBit3[1] == 0)
        {
          bleGamepad.press(BUTTON_12);
          currentButtonStates[2] = 1;
        }
        else
        {
          bleGamepad.release(BUTTON_12);
          currentButtonStates[2] = 0;
        }

        //------- A Button -------//

        if (dataArrayBit2[1] == 0)
        {
          bleGamepad.press(BUTTON_4);
          currentButtonStates[3] = 1;
        }
        else
        {
          bleGamepad.release(BUTTON_4);
          currentButtonStates[3] = 0;
        }

        //------- B Button -------//

        if (dataArrayBit0[1] == 0)
        {
          bleGamepad.press(BUTTON_1);
          currentButtonStates[4] = 1;
        }
        else
        {
          bleGamepad.release(BUTTON_1);
          currentButtonStates[4] = 0;
        }

        //------- C Button -------//

        if (dataArrayBit1[1] == 0)
        {
          bleGamepad.press(BUTTON_2);
          currentButtonStates[5] = 1;
        }
        else
        {
          bleGamepad.release(BUTTON_2);
          currentButtonStates[5] = 0;
        }

        //------- X Button -------//

        if (dataArrayBit6[1] == 0)
        {
          bleGamepad.press(BUTTON_7);
          currentButtonStates[6] = 1;
        }
        else
        {
          bleGamepad.release(BUTTON_7);
          currentButtonStates[6] = 0;
        }

        //------- Y Button -------//

        if (dataArrayBit5[1] == 0)
        {
          bleGamepad.press(BUTTON_5);
          currentButtonStates[7] = 1;
        }
        else
        {
          bleGamepad.release(BUTTON_5);
          currentButtonStates[7] = 0;
        }

        //------- Z Button -------//

        if (dataArrayBit4[1] == 0)
        {
          bleGamepad.press(BUTTON_8);
          currentButtonStates[8] = 1;
        }
        else
        {
          bleGamepad.release(BUTTON_8);
          currentButtonStates[8] = 0;
        }

        //------- LT Button -------//

        if (dataArrayBit3[2] == 0)
        {
          //bleGamepad.press(BUTTON_9);
          //bleGamepad.setBrake(32767);
          bleGamepad.setBrake(255);
          currentButtonStates[9] = 1;
        }
        else
        {
          //bleGamepad.release(BUTTON_9);
          bleGamepad.setBrake(0);
          currentButtonStates[9] = 0;
        }

        //------- RT Button -------//

        if (dataArrayBit7[1] == 0)
        {
		  //bleGamepad.setAccelerator(32767);
          bleGamepad.setAccelerator(255);
          currentButtonStates[10] = 1;
        }
        else
        {
          //bleGamepad.release(BUTTON_10);
          bleGamepad.setAccelerator(0);
          currentButtonStates[10] = 0;
        }
       
       /*
        // Latency Test Pin
        // Intended for BlueRetro latency test pad to measure BLEGamepad update speeds without modifying the Controller
		// 
		// Use print enabled to test your setup, then disable the prints as they will impact latency negatively
        if (digitalRead(latPin) == LOW)
        {
          Serial.print("Latency Test Press");
          bleGamepad.press(BUTTON_3);
          currentButtonStates[2] = 1;
        }
        else
        {
          bleGamepad.release(BUTTON_3);
          currentButtonStates[2] = 0;
        }
        */
      }
      // End Digital Mode - Process Input
      // ================================

	  // RTOS Watermark check
    // unsigned int wMark2 = uxTaskGetStackHighWaterMark(nullptr);
    // printf("Watermark - SendReport B loop %u\n", wMark2);

    // All Input states gathered Send Update if state has changed
	  // This is how blegampad library runs the check but you can't compare arrays like this so condition is always true
	  // Substantially higher power consumption as a result of constant transmit    
       if (currentButtonStates != previousButtonStates)
      {
        for (byte currentIndex = 0; currentIndex < numOfButtons; currentIndex++)
      // To do this the correct way - itterate over each index but it has problems with loss of single packet
      //if (currentButtonStates[currentIndex] != previousButtonStates[currentIndex]) { 
      //  for (byte currentIndex = 0; currentIndex < numOfButtons; currentIndex++){
        {
          previousButtonStates[currentIndex] = currentButtonStates[currentIndex];
        }
        bleGamepad.sendReport();
        bleTimer = timeStamp;
        //bleTimer = millis();
      }
    }


      // Invalid Data or no connection error state
      // =========================================
      // Communication or wiring issue
    
    else
    {
      for (int printByteCounter = 0; printByteCounter < modeCounter + 1; printByteCounter++)
      {
        int derpvar;
        derpvar = 0;
        // Serial.println("Error: Gamepad may not be paired OR Pinout/Timing Incorrect");
        // Uncomment lines for additional debug output
        // Serial.print("  -- Byte ");
        // Serial.print(printByteCounter);
        // Serial.print(" - ");
        // Serial.print(dataArrayBit0[printByteCounter]);
        // Serial.print(dataArrayBit1[printByteCounter]);
        // Serial.print(dataArrayBit2[printByteCounter]);
        // Serial.print(dataArrayBit3[printByteCounter]);
        // Serial.print(dataArrayBit4[printByteCounter]);
        // Serial.print(dataArrayBit5[printByteCounter]);
        // Serial.print(dataArrayBit6[printByteCounter]);
        // Serial.print(dataArrayBit7[printByteCounter]);
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
  if (TLACK == HIGH and nibble0Read == 0)
  {
    digitalWrite(THPin, LOW);
    delayMicroseconds(4); // I don't think I need this
    digitalWrite(TRPin, LOW);
    delayMicroseconds(4);
    // delayMicroseconds(1);
  }

  // If the 1st nibble is being sent and we havn't read the 1st nibble yet get the data from the pins
  if (TLACK == LOW and nibble0Read == 0)
  {
    // Sets ready to read the 1st nibble of data
    digitalWrite(TRPin, HIGH);
    delayMicroseconds(8); // For ESP32 with lower setting get random button press

    // Read the Data for the 1st Nibble
    dataArrayBit0[byteCounter] = digitalRead(dataPinD0);
    dataArrayBit1[byteCounter] = digitalRead(dataPinD1);
    dataArrayBit2[byteCounter] = digitalRead(dataPinD2);
    dataArrayBit3[byteCounter] = digitalRead(dataPinD3);

    if (dataArrayBit0[byteCounter] > 1)
    {
      dataArrayBit0[byteCounter] = 0;
    }
    if (dataArrayBit1[byteCounter] > 1)
    {
      dataArrayBit1[byteCounter] = 0;
    }
    if (dataArrayBit2[byteCounter] > 1)
    {
      dataArrayBit2[byteCounter] = 0;
    }
    if (dataArrayBit3[byteCounter] > 1)
    {
      dataArrayBit3[byteCounter] = 0;
    }

    // 1st Nibble has been read
    nibble0Read = 1;

    // Check if we are reading data for Analogue mode or Digital Mode
    // The mode defines the number of Bytes we need to read
    if (byteCounter == 0)
    {
      if ((dataArrayBit0[0] == 1 and dataArrayBit1[0] == 0 and dataArrayBit2[0] == 0 and dataArrayBit3[0] == 0) or (dataArrayBit0[0] == 0 and dataArrayBit1[0] == 1 and dataArrayBit2[0] == 1 and dataArrayBit3[0] == 0) or (dataArrayBit0[0] == 1 and dataArrayBit1[0] == 1 and dataArrayBit2[0] == 1 and dataArrayBit3[0] == 0))
      {
        modeCounter = AnalogMode;
      }
      else
      {
        modeCounter = DigitalMode;
      }
    }
  }

  // Check if the 2nd Nibble is being sent and if we have already read the 1st Nibble
  if (TLACK == HIGH and nibble0Read == 1)
  {
    // Sets ready to read the 2nd Nibble
    digitalWrite(TRPin, LOW);
    delayMicroseconds(8);
    // Read the data for the 2nd Nibble
    dataArrayBit4[byteCounter] = digitalRead(dataPinD0);
    dataArrayBit5[byteCounter] = digitalRead(dataPinD1);
    dataArrayBit6[byteCounter] = digitalRead(dataPinD2);
    dataArrayBit7[byteCounter] = digitalRead(dataPinD3);
    if (dataArrayBit4[byteCounter] > 1)
    {
      dataArrayBit4[byteCounter] = 0;
    }
    if (dataArrayBit5[byteCounter] > 1)
    {
      dataArrayBit5[byteCounter] = 0;
    }
    if (dataArrayBit6[byteCounter] > 1)
    {
      dataArrayBit6[byteCounter] = 0;
    }
    if (dataArrayBit7[byteCounter] > 1)
    {
      dataArrayBit7[byteCounter] = 0;
    }

    // Reset the 1st Nibble read variable, ready for the next Byte of data
    nibble0Read = 0;

    // Increase the Byte counter by 1 so we are ready to read the next Byte of data
    byteCounter++;
  }

  // Time Stamp Routines to check if data has paused.
  currentMillis = millis();

  if (currentMillis - timeStamp >= interval)
  {
    // Resetting All Variables
    timeStamp = millis();
    digitalWrite(THPin, HIGH);
    digitalWrite(TRPin, HIGH);
    byteCounter = 0;
    nibble0Read = 0;
    Serial.println("Init Controller Protocol...");
  }
}
//=============================
// End Controller related items



// CPU Speed related items
//=============================
//Set CPU Speed
//
// This will heavily impact GPIO speed and protocol level checkins with the controller
// Seriously.
// For more information see the following link where this code was shamelessly borrowed from
// https://deepbluembedded.com/esp32-change-cpu-speed-clock-frequency/ 
//
void setCpu()
{
//This sets the CPU frequency
//Possible frequencies are directly tied to your own device so dont just yolo values in here and then cry
//
//function takes the following frequencies as valid values:
//  240, 160, 80    <<< For all XTAL types
//  40, 20, 10      <<< For 40MHz XTAL
//  26, 13          <<< For 26MHz XTAL
//  24, 12          <<< For 24MHz XTAL
  //Print initial Speed
  Freq = getCpuFrequencyMhz();
  Serial.print("Current CPU Freq = ");
  Serial.print(Freq);
  Serial.println(" MHz");
  delay(30);
  Serial.end();  //Required for ESP bug
  setCpuFrequencyMhz(40);
  delay(30);
  Serial.begin(115200);  // Required for ESP bug
  delay(30);
  Freq = getCpuFrequencyMhz();
  Serial.print("New CPU Freq = ");
  Serial.print(Freq);
  Serial.println(" MHz");
}

//Read CPU Speed and Clock
void getCPU()
{
  Freq = getCpuFrequencyMhz();
  Serial.print("Boot CPU Freq = ");
  Serial.print(Freq);
  Serial.println(" MHz");
  Freq = getXtalFrequencyMhz();
  Serial.print("My XTAL Freq = ");
  Serial.print(Freq);
  Serial.println(" MHz");
  Freq = getApbFrequency();
  Serial.print("My APB Freq = ");
  Serial.print(Freq);
  Serial.println(" Hz");
 }
//=============================
// End CPU Speed related items


// This is the main loop due to threading in arduino + core tasking in RTOS
//==========================================================================
void setup()
{


  // Start BLE Gamepad
  BleGamepadConfiguration bleGamepadConfig;
  bleGamepadConfig.setControllerType(CONTROLLER_TYPE_GAMEPAD); // CONTROLLER_TYPE_JOYSTICK, CONTROLLER_TYPE_GAMEPAD (DEFAULT), CONTROLLER_TYPE_MULTI_AXIS
  //bleGamepadConfig.setControllerType(CONTROLLER_TYPE_MULTI_AXIS);  //Axis will break Windows Gamepad test function but may work in games
  bleGamepadConfig.setButtonCount(numOfButtons);
  bleGamepadConfig.setWhichAxes(enableX, enableY, enableZ, enableRX, enableRY, enableRZ, enableSlider1, enableSlider2); // Can also be done per-axis individually. All are true by default
  bleGamepadConfig.setHatSwitchCount(numOfHatSwitches);                                                                 // 1 by default

  //Trigger settings
  //Emulate Xbox behavior to avoid rY and rZ use
  bleGamepadConfig.setIncludeAccelerator(true);
  bleGamepadConfig.setIncludeBrake(true);

  //Modify HID Range Values
  // WARNING: This impacts all MAP statements and Im lazy so it's not just defined as global variable 

  //
  //Set Range default -32k to +32k
  //bleGamepadConfig.setAxesMin(0x8001);  // -32767 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal
  //bleGamepadConfig.setAxesMax(0x7FFF);   // 32767 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal
  //
  //== Set Range Axes Range
  // Emulate Xbox 0-255
  bleGamepadConfig.setAxesMin(0x00);       //Set Min to 0 //try 0 as min similar to xbox
  //bleGamepadConfig.setAxesMax(0xFF);       //Set Max to 255 // try 255 as Max similar to xbox  //removed leading 00s
  ///== Set Trigger Range
  // Emulate Xbox 0-255
  bleGamepadConfig.setSimulationMin(0x00);
  //bleGamepadConfig.setSimulationMax(0xFF);  //Max 255 //This doesn't seem to work right for BLEgamepad

  bleGamepadConfig.setAxesMin(0x0001); // -32767 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal
  //
  // if blegamepad for Accel and Bake don't report try this instead 
  //bleGamepadConfig.setWhichSimulationControls(enableRudder, enableThrottle, enableAccelerator, enableBrake, enableSteering); 

  //==Adjust Vid and PID
  // These can be set to advertise various vendor and product IDs from other controllers
  // bleGamepadConfig.setVid(0x45e); 
  // bleGamepadConfig.setPid(0x2e0);

  // ==Other Available Special Button Options
  // Intermittent results with these "special buttons" across all tested remote devices
  //
  // bleGamepadConfig.setIncludeStart(true);
  // bleGamepadConfig.setIncludeSelect(true);
  // bleGamepadConfig.setIncludeMenu(true);
  // bleGamepadConfig.setIncludeHome(true);
  // bleGamepadConfig.setIncludeBack(true);
  // bleGamepadConfig.setIncludeVolumeInc(true);
  // bleGamepadConfig.setIncludeVolumeDec(true);
  // bleGamepadConfig.setIncludeVolumeMute(true);
  //
  // ==Use or disable BLEGampead AutoReporting
  // AutoReport can impact latency negatively with high speed of updates
  bleGamepadConfig.setAutoReport(false); // This is true by default

  //Use all the bleGamepad Settings to initiate controller
  bleGamepad.begin(&bleGamepadConfig);

  // Setting input and output pins for Comms with Controller
  // Use Heavy and Lightwing pinout settings instead of changing this
  //==========================// Pin 1 = +5V
  pinMode(dataPinD1, INPUT); // Pin 2 = Data D1
  pinMode(dataPinD0, INPUT); // Pin 3 = Data D0
  pinMode(THPin, OUTPUT);    // Pin 4 = TH Select
  pinMode(TRPin, OUTPUT);    // Pin 5 = TR Request
  pinMode(TLPin, INPUT);     // Pin 6 = TL Response
  pinMode(dataPinD3, INPUT); // Pin 7 = Data D3
  pinMode(dataPinD2, INPUT); // Pin 8 = Data D2
  //==========================// Pin 9 = GND

  // Set pin state to begin reading controller state
  digitalWrite(THPin, HIGH);
  digitalWrite(TRPin, HIGH);

  // Define Latency test pin
  pinMode(latPin, INPUT_PULLUP);

  // Setup Serial comms for Serial Monitor
  Serial.begin(115200);
  Serial.println("Device Booting....");
}

// Loop will always run on Core 1
void loop()
{
  //Settings related to initial boot of device
  //This doesn't get called on wake from deep sleep
  if (firstBoot == 0){
    getCPU();
    //setCpu();
    firstBoot=1;
    }

  pollController();
}
