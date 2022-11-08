# This is under currently construction.

# Forward
If you are chasing low latency controllers and have never tested your display lag, you are approaching this *completely backwards*. If you are trying to blame controller latency before video as the culprit for how poorly you play games, reconsider. LAG is cumulative from input to output as far as your perception is concerned.

Some of the better projectors for example alone add 30-70ms of lag. It's not uncommon to see with a series of connected devices in path 40-60ms before hitting your "2ms" LCD. Even your LCD is unlikely to perform at advertised speeds outside of VERY specific configurations. Wrong setting, add 18+ms.  That's omitting any upscaler, AV Reciver in the middle, media converters, hdmi switches, etc - each of which may have setting based penalties. 

[RetroRGB](https://www.retrorgb.com/) has done extensive testing and how-to videos on the subject.


# Controller Latency Testing in General
Before we dive into the BLEGamepad/BlueRetro stuff, it's useful to understand how this testing is done in the wild elsewhere. Consistency is important across numerous controllers and setups. The best place to start is with the MiSTER. 

## What you need:
- [x] A controller      - This acts as the "Transmitter" for the keypress

- [x] Arduino Pro Micro - This performs time based calculation for Press->Recieve

- [x] Your reciver      - This is the "Recieving" device 

## Detailed guide on how Latency testing is performed on the Mister Platform:
https://www.cathoderayblog.com/lag-test-your-controller-mister-fpga-input-latency-tester/

I strongly advise taking a look over the content above as it will make everything from here much easier to understand. There are also a fair amount of videos floating around about this. Effectively you're using an arduino to "press" a button, and then another pin which will detect how fast that button is recieved on the other end.  The arduino acts as a timer to count the time from when the press is sent to the time it is received and starts the next loop. After *10,000+* itterations, you have a good average input latency.

## A big Mister Latency Controller test List:
https://rpubs.com/misteraddons/inputlatency

This is a list of controller input latency tests providing a useful comparison across a wide array of controllers as to what is "low latency", whats "Normal" and what is unbearably slow. 


# Latency Testing with [BlueRetro](https://github.com/darthcloud/BlueRetro)
Darthcloud has included in BlueRetro a method to perform testing with the same arduino code that Mister uses. In brief you have to use the correct firmware, set the correct profile, and then use the right GPIO pin to connect to your arduino. You can read some more on this from his dev journal [here](https://hackaday.io/project/170365-blueretro/log/187443-2020-12-26-update-latency-tests-release-v010).

There are some caveats you should be aware of I have spent *many* hours beating my head on a desk over.

- [x] You **MUST** use the "parallel_1P_external.bin" when flashing firmware 
 - Console specific or embedded firmwares wont work
 - DO NOT connect your BR reciever to a console (only external power)
	  
- [x] Before connecting your controller you **MUST** enable the "Latency Test" profile
 - If you fail to do this before connecting the controller you will have to power off the controller and reset blueretro or wait some time
 - Note: This often does not survive power down
	  
- [x] You **MUST** have access to GPIO26 on your BlueRetro testing device.  
 - This could be risky if you only have console specific versions that require modification for output pin and the external power (eg. Pre-built N64 Adapter)

- [x] You **MUST** have access to the PCB of your controller to solder on a wire for arduino to "press" your testing button
 - This typically requires modification to your controller PCB, like scraping away the coating to hit a copper trace.
 - This is also why my Lightwing PCBs have a dedicated pin on them to avoid damage.


# Latency Testing with [ESP32-BLE-Gamepad](https://github.com/lemmingDev/ESP32-BLE-Gamepad)
ESP32-BLE-Gamepad does not contain any "specific" latency testing function, you simply define a button that works when pulled low and to connect it to the arduino. In the code linked this pin should connect to arduino pin5.

For example
```
      if (digitalRead(latPin) == LOW) { //when arduino pulls this pin low send button press
        bleGamepad.press(BUTTON_1);
      } else {
        bleGamepad.release(BUTTON_1);
      }
```

# Latency test your ESP32-BLE-Gamepad controller with BlueRetro
 I have used "BR" in place of BlueRetro below

###  The Arduino Pro Micro
 1. Flash your Arduino with the [latency test code](https://github.com/misteraddons/inputlatency/blob/main/arduino/MiSTer_USB_Latency_Test_Lemonici/MiSTer_USB_Latency_Test_Lemonici.ino)

 2. Unplug your Arduino

###  The Controller
  3. Upload your ESP32-BLE-Gamepad code to your controller
 
  4. Turn the controller OFF

  5. Solder a wire to your controller pin to use for testing
 
  6. Connect the wire soldered to the controller to Arduino Pin 5
 
###  BlueRetro
  7. [Flash the "parallel_1P_external.bin"](https://github.com/darthcloud/BlueRetro/wiki/BlueRetro-DIY-Build-Instructions) to your BlueRetro reciever and power cycle it
 
  8. Connect GPIO26 (some boards may show as IO26) on BR to Pin 2 of the Arduino
 
  9. Using **Chrome Browser** [connect to BR over bluetooth](https://hackaday.io/project/170365-blueretro/log/180020-web-bluetooth-ble-configuration-interface) and set the mapping profile to "Latency Test" and click save 

 - Note:  There is a "Test" profile in the second box - do not use this.

 - Example of the correct setting:
![the right way](https://user-images.githubusercontent.com/106001964/181081926-28c2eff5-fb45-421e-913e-e1e7c2fb68aa.jpg)
     
 ### Start Latency Testing    
 10. Connect to the serial console of your arduino com port (115200 - 8 n 1) 
  - Failure to connect to serial will not allow the code to start running
     
 11. Power on your controller

 12. Check your Serial console to the arduino and confirm it is incrimenting. If so Allow 10k+ itterations to complete to get your latency data.
