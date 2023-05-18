# BLE-3D-Saturn-Public
Sega Saturn 3D Controller Bluetooth(BLE) Adapter

# BLE-3D #

## **Background** ##
For years I've dreamed of a wireless Sega Saturn 3d Pad, just like the patents intended.

- US Patent Number US 7488,254 B2 https://patentimages.storage.googleapis.com/a2/97/47/e77a8c63165461/US7488254.pdf
- US Patent Number Des. 409, 149   https://patentimages.storage.googleapis.com/3d/96/ad/edd675699738cf/USD409149.pdf
- European Patent Number EP 1 332 778 B1 https://patentimages.storage.googleapis.com/ed/ba/17/c22422d5d3cc47/EP1332778B1.pdf
![An inspiration image I have been using in development](https://i.imgur.com/Myag1Ka.png) 

Fast forward many years, I stumbled across some work [Hexfreq](https://twitter.com/hexfreq) was doing with arduino and asked if I could take a look at the code. The rest is history.
From there [Humble Bazooka](https://twitter.com/humblebazooka) and I have spent countless hours to make sure this was a reality. darthcloud has also been an immense help along the way.

## **PCBs** ##
There are 2 publicly availble DIY pcb's for "dev kits". There are various issues with them and aren't intended for retail use, but you can build one yourself today.

**Heavywing**
- The original PCB. Take the above patents put them in a blender. It adds LCD Support, Motion Controls & Rumble. You can use the Lightwing code (better performance) on a Heavywing PCB but will need to change some pin mappings.
- PCB: https://oshpark.com/shared_projects/ki7HbZV4
<BR> <img src="https://i.imgur.com/oe6XZfD.png" height="400"/> <BR>

**Lightwing**
 - This is a 'new' revision of the BLE-Saturn-3D that focuses on core functionality. This was optimized as the low cost option for home builders. At current, this is the advised version to use until development can progress farther on the extra features of the HeavyWing 
- PCB: https://oshpark.com/shared_projects/7RqwDWJT 
<BR><img src="https://i.imgur.com/UOWCkPk.jpg" height="400"/> <img src="https://i.imgur.com/fSMhvSw.png" height="400"/> <BR>


## **BOM** ##
 ## Lightwing ##
 - 1x Lightwing PCB
 - 1x [Adafruit Feather Huzzah32](https://www.adafruit.com/product/3405)
 - 1x Battery

## Heavywing ##
 - 1x Heavywing PCB
 - 1x [Adafruit Feather Huzzah32](https://www.adafruit.com/product/3405) 
 - 1x [HiLetgo GY-521 Tilt Sensor](http://hiletgo.com/ProductDetail/2157948.html)* AKA MPU-6050 (optional - while it does "work" the kalman filter needs some rework. Must support 3.3v, some are 5v only) 
- 1x OLED 128X64 SSD1306 (optional)
- 1x 5v Rumble Motor* (optional - this circuit is not populated on the PCB currently)
- 1x Battery

## **Assembly** ##
Assembly is straightforward, it's just a PCB sandwhich and some solder but orientation might be counter intuitive.
 ## Lightwing ##
   - Pictures to be uploaded soon

  
 ## **Heavywing** ##
   - *Pictures to be uploaded soon*
  
  
## **Before you compile the code** ##
If you are mixing and matching lightwing code with the heavywing PCB, make sure you change the pinmapping. 

This is well documented in the code if you jump to "Pinmappings for your PCB version"


## **Caveats and Gotchas** ##
 - ## Battery Warning ##
 It is *extremely* important that you check the polarity on your battery jack is oriented correctly. There is 0 polarity protection. The charging circuit WILL fry if you plug it in and it's backwards.

 - ## Powering Down ##
 Due to the design of the adafruit feather (esp32) there is no formal power switch. You either need to solder a physical switch on to the battery, or unplug it. This is not always easy with the JST connector used so be careful.
 
  - ## Charging ##
 Another unfortunate aspect of the power circuit design on the feather is that the device must be powered on to charge. There is no "charging mode" so anytime the controller needs to charge it will be broadcasting. Deep sleep is the best solution for this long term but there are reassons it's not included just yet.

  - ## Missing Trigger ##
 In windows if you go to look at the button inputs the right trigger is not displayed in joy.cpl.  The trigger is there, but you have to use another pad tester to "see it". If your trigger is not shown like this image, that's expected and appears to be a problem with joy.cpl in particular.


## **Compatibility** ##
 This is an area I'm extremely proud of as of writing the BLE3D is known to work with the following. 
 
  - Windows, Linux, Steamdeck
  - ALL supported Blueretro https://github.com/darthcloud/BlueRetro consoles 
  - That means the 3D is now available on (in no particular order)
    - NeoGeo, Supergun, JAMMA
	- Atari 2600/7800, Master System
	- NES, PCE / TG16, Mega Drive / Genesis, SNES
	- CD-i, 3DO, Jaguar, Saturn, PSX, PC-FX, 
	- JVS (Arcade), Virtual Boy, N64, Dreamcast, PS2, GameCube & Wii
	- I'm sure I'm missing a handful
 
  - Note the analog pad/triggers support are per console, there is no 'digital emulation' on the analog stick.


## Various Demos and feature teasers ##
Taking Nights for a spin over Bluetooth https://twitter.com/GamingNJncos/status/1537364206881755136
LCD and customizable Boot Logo https://twitter.com/GamingNJncos/status/1532819934551724033
Pad Test Demo in X-Men COTA https://twitter.com/GamingNJncos/status/1537362477817765889
Steamdek test https://twitter.com/GamingNJncos/status/1540845741710745602


## **Note on the Boot Logos (only in heavywing)** ##
This is documented in the code however if you want to convert images this tool is really useful https://lcd-image-converter.riuson.com/en/about/ 


## Whats with the Names? ##
Having some background on the feather platform, I went this route for personal ease in development.  Feather add-ons or expansions (hat equivalent for raspberry pi) are called "Wings". Combine that with some Sega nerd-lore and you get the light and heavywing
https://panzerdragoon.fandom.com/wiki/Heavy_Wing
https://panzerdragoon.fandom.com/wiki/Light_Wing


## **Strange notes and pedantic details. ** ##
 ## Protocol Details and Resources ##
 - A great overview across numerous sega consoles https://hackaday.io/project/170365-blueretro/log/180790-evolution-of-segas-io-interface-from-sg-1000-to-saturn
 - Additional protocol details for the Saturn http://forums.modretro.com/index.php?threads/saturn-controller-protocol-mk80116-mk80100.11328/

 ## Saturn 3D supported Game list, patent details, and more ##
https://segaretro.org/3D_Control_Pad
 
 ## Polling oddity or Opportunity? ## 
The saturn only polls the 3D protocol at 16ms intervals (screen blank). There is however some suggestion in the below post that the Action Replay (this is firmware version specific) can attempt polling faster.  It's worth taking a look at as it may be possible to force games into a faster mode with patching.

https://nfggames.com/forum2/index.php?topic=5055.0

"For whatever reason, it's only polling the controller for 14.25ms, with around 310us pauses in between, so it's polling much quicker, but the real issue is after this 14.25ms is up, it then drives the SEL line Hi and goes about it's business for ~2.5ms after that, then back to polling."

## Other demos and informational links ##
A DIY internal BlueRetro receiver for Saturn https://twitter.com/nosIndulgences/status/1573719805496299520
Hexfreq showing off a completely differnt bluetooth saturn solution https://twitter.com/hexfreq/status/1468282054978818062

## **Libraries in use** ##
https://github.com/lemmingDev/ESP32-BLE-Gamepad
 - This is subject to change long term as new and improved libraries emerge to support BLE HID gamepads

https://github.com/h2zero/NimBLE-Arduino
