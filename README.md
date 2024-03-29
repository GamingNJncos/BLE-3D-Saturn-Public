# BLE-3D #
Sega Saturn 3D Controller Bluetooth(BLE) Adapter

# **What does it do?** #
- This is an open source and completely solderless adapter for the Sega Saturn 3D Controller that converts it to a Bluetooth(BLE) HID Gamepad. 
- The end result demonstrates a battery powered and solderless adapter that allows you to use one of the best controllers ever made to date across numerous consoles

### Highlights ###
- **Zero modifications to the factory controller** are neccesary to build, test, or play with this adapter.
- Both Analog, and Digital mode (switch on controller) are fully supported
- Want to revert back to OE? Simply plug the OE cable back in. 
- Latency testing pin on-pcb offers consistent and reproducible testing across code changes and development

### Lowlights ###
- This isn't a "retail ready product", it's a proof of concept
- No case for this has been published yet (bare pcb)
- Power circuit has a couple issues. More details in Caveats section


# **Compatibility** #
 This is an area I'm extremely proud of as of writing the BLE3D is known to work with: 
  - Windows, Linux, and Steamdeck
  - ALL supported [Blueretro](https://github.com/darthcloud/BlueRetro) consoles 
  - That means the 3D is now available on (in no particular order)
        - NeoGeo, Supergun, JAMMA
	- Atari 2600/7800, Master System
	- NES, PCE / TG16, Mega Drive / Genesis, SNES
	- CD-i, 3DO, Jaguar, Saturn, PSX, PC-FX, 
	- JVS (Arcade), Virtual Boy, N64, Dreamcast, PS2, GameCube & Wii
	- I'm sure I'm missing a handful
 
  - Note: the analog pad/triggers support are per console, there is no 'digital emulation' on the analog stick.


# **Background** #
For years I've dreamed of a wireless Sega Saturn 3d Pad, just like the patents intended.

- [US Patent Number US 7488,254 B2](https://patentimages.storage.googleapis.com/a2/97/47/e77a8c63165461/US7488254.pdf)
- [US Patent Number Des. 409, 149](https://patentimages.storage.googleapis.com/3d/96/ad/edd675699738cf/USD409149.pdf)
- [European Patent Number EP 1 332 778 B1](https://patentimages.storage.googleapis.com/ed/ba/17/c22422d5d3cc47/EP1332778B1.pdf)
![An inspiration image I have been using in development](https://i.imgur.com/Myag1Ka.png) 

Fast forward many years, I stumbled across some work [Hexfreq](https://twitter.com/hexfreq) was doing with arduino and asked if I could take a look at the code. The rest is history.
From there [Humble Bazooka](https://twitter.com/humblebazooka) and I have spent countless hours to make sure this was a reality. I'd be remissed to leave out [darthcloud](https://twitter.com/darthcloud64) has also been an immense help along the way.

# **PCBs** #
- There are 2 publicly available DIY pcb's for "dev kits". 
- There are various issues or design misses with both pcbs
- They aren't intended for retail use, but you can build one yourself **today**.
- Beveling the controller facing pins is advised 

## **LightWing** ##
 - This is a 'new' revision of the BLE-Saturn-3D that focuses on core functionality. This was optimized as the low cost option for home builders. At current, this is the advised version to use until development can progress farther on the extra features of the HeavyWing 
- PCB: https://oshpark.com/shared_projects/7RqwDWJT 
<BR><img src="https://i.imgur.com/UOWCkPk.jpg" height="400"/> <img src="https://i.imgur.com/fSMhvSw.png" height="400"/> <BR>

## **HeavyWing** ##
- The original PCB. Take the above patents put them in a blender. It adds LCD Support, Motion Controls & Rumble. You can use the LightWing code (better performance) on a Heavywing PCB but will need to change some pin mappings.
- PCB: https://oshpark.com/shared_projects/ki7HbZV4
<BR> <img src="https://i.imgur.com/oe6XZfD.png" height="400"/> <BR><BR> 

# **BOM** #
## LightWing ###
 - 1x [LightWing PCB](https://oshpark.com/shared_projects/7RqwDWJT)
 - 1x [Adafruit Feather Huzzah32](https://www.adafruit.com/product/3405)
 - 1x Battery
<BR><BR> 
	
## HeavyWing ##
 - 1x [HeavyWing PCB](https://oshpark.com/shared_projects/ki7HbZV4)
 - 1x [Adafruit Feather Huzzah32](https://www.adafruit.com/product/3405) 
 - 1x [HiLetgo GY-521 AKA MPU-6050 Tilt Sensor](http://hiletgo.com/ProductDetail/2157948.html)*  (optional - while it does "work" the kalman filter needs some rework. Must support 3.3v, some are 5v only) 
- 1x OLED 128X64 SSD1306 (optional)
- 1x 3.3v Rumble Motor* (optional - this circuit is not populated on the PCB currently)
- 1x Battery
<BR><BR> 

	
# **Assembly** #
- Assembly is straightforward, it's just a PCB sandwhich and some solder
- Orientation might be counter intuitive

 ### **LightWing - Assembly** ###
- When you build this lay Segata Face-Down, then set the Feather through the holes with the ESP, USB, and Battery plug facing you
- This provies some additional space for the battery directly under the PCB which would closet match the original patent docs visually
<BR><img src="https://i.imgur.com/34AWDPJ.png" height="400"/> <img src="https://i.imgur.com/f7SZWjN.png" height="400"/> <BR>

 ### **LightWing - Installation** ###
- Sega Logo and Button Face top
- Segata faces triggers or back of controller
<BR> <img src="https://i.imgur.com/QNJOK90.jpg" height="400"/> <img src="https://i.imgur.com/H4lsmWy.jpg" height="400"/> <BR>  <BR>

  ### **HeavyWing - Assembly** ###
   - *Pictures to be uploaded soon*
  
<BR><BR>  
# **Before you compile the code** #
- [PlatformIO installed in VScode](https://docs.platformio.org/en/stable/tutorials/espressif32/arduino_debugging_unit_testing.html), the [platformio.ini](https://github.com/GamingNJncos/BLE-3D-Saturn-Public/blob/main/platformio.ini), and the main.cpp for the Lightwing or the Heavywing
- This has not been tested with Arduino IDE extensively but should compile fine however extremely slow
- If you are mixing and matching LightWing code with the HeavyWing PCB, make sure you change the pinmapping. 
- This is well documented in the code if you jump to "Pinmappings for your PCB version"

## Powering the Device ##
This is direct from the [Adafruit Documentation](https://learn.adafruit.com/adafruit-huzzah32-esp32-feather/power-management)
- There's two ways to power a Feather:
- **You can connect with a USB cable** (just plug into the jack) and the Feather will regulate the 5V USB down to 3.3V.
-  **You can also connect a 4.2/3.7V Lithium Polymer** (LiPo/LiPoly) or Lithium Ion (LiIon) battery to the JST jack. This will let the Feather run on a rechargeable battery.
-  **When the USB power is powered, it will automatically switch over to USB for power**, as well as start charging the battery (if attached). 
- **This happens 'hot-swap' style** so you can always keep the LiPoly connected as a 'backup' power that will only get used when USB power is lost.

<BR><BR> 
# **Caveats and Gotchas** #
 ## A Note on the Huzzah32 revisions ##
  - Check your WROOM version before flashing!
  - There is a NEW revision of the huzzah32 shipping with the Wroom-32-**E** that will have some problems initially when plugged into the controller
  - This is a result of changes to pin IO12 between the D and E modules (and a bootstrap pin) 
  - PCB's were made based on result on the Wroom-32-**D** module and have no issues "out of the box"
  
  - On Wroom-E when the pcb is plugged into the controller at boot similar to boot:0x33 (SPI_FAST_FLASH_BOOT) **invalid header: 0xffffffff**
  - You can burn an efuse to work around this with the espefuse tool. Note this is irreversible, if you are uncomfortable with this try to find another vendor with the 32D in stock   - How to burn the efuse: `espefuse.py --port [com port] --baud 115200 set_flash_voltage 3.3V` confirm the fuse status with `espefuse -p [com port] --baud 115200 summary`. At the bottom it will say **Flash voltage (VDD_SDIO) set to 3.3V by efuse.**
- At the bottom it will say **Flash voltage (VDD_SDIO) set to 3.3V by efuse.**

 ## Battery Warning ##
  - It is *extremely* important that you check the polarity on your battery jack is oriented correctly. There is 0 polarity protection. The charging circuit WILL fry if you plug it in and it's backwards.

 ## Powering Down ##
  - Due to the design of the adafruit feather (esp32) there is no formal power switch. 
  - You must either solder a physical switch on to the battery, **or** unplug it. This is not always easy with the JST connector used so be careful.
 
  ## Charging ##
   - Another unfortunate aspect of the power circuit design on the feather is that the device must be powered on to charge. 
   - There is no "charging mode" so anytime the controller needs to charge it will be broadcasting. 
   - Deep sleep is the best solution for this long term but there are reassons it's not included just yet.

  ## Missing Trigger ##
   - In windows if you go to look at the button inputs in joy.cpl the right trigger is not displayed.  
   - The trigger is there, but you have to use another pad tester to "see it".
  - The picture below *is the expected behavior* and appears to be a problem with joy.cpl in particular.
	<BR><img src="https://i.imgur.com/XALRnzy.png" height="400"/> <BR> <BR>


## Various Demos and feature teasers ##
- [Taking Nights for a spin over Bluetooth](https://twitter.com/GamingNJncos/status/1537364206881755136)
- [LCD and customizable Boot Logo](https://twitter.com/GamingNJncos/status/1532819934551724033)
- [Pad Test Demo in X-Men COTA](https://twitter.com/GamingNJncos/status/1537362477817765889)
- [Steamdek Test](https://twitter.com/GamingNJncos/status/1540845741710745602)
- [Early External Unit Testing](https://twitter.com/GamingNJncos/status/1404144038693986307)

## Latency Testing ##
- A significant amount of consideration went into making it easy to test code changes and the impact on latency
- The Lightwing PCB has a dedicated solder pad for consistent testing methodology and code incorporates easy to toggle enable/disable for the pin
- Documentation on the process is available [here](https://github.com/GamingNJncos/BLE-3D-Saturn-Public/tree/main/BlueRetro_Latency_Testing)

## **Note on the Boot Logos (Supported on HeavyWing PCB)** ##
This is documented in the code however if you want to convert images [this tool](https://lcd-image-converter.riuson.com/en/about/) is really useful  
<BR> <img src="https://i.imgur.com/MSQY0Gh.jpg" height="400"/> <BR>


## Whats with the Names? ##
- Feather add-ons or expansions (hat equivalent for raspberry pi) are called "Wings". 
- Combine that with some Sega nerd-lore and you the [Heavy Wing](https://panzerdragoon.fandom.com/wiki/Heavy_Wing) and [Light Wing](https://panzerdragoon.fandom.com/wiki/Light_Wing)

<BR><BR>

# **Strange notes and pedantic details** #
 ## Protocol Details and Resources ##
 - [A great overview across numerous sega consoles](https://hackaday.io/project/170365-blueretro/log/180790-evolution-of-segas-io-interface-from-sg-1000-to-saturn) by [darthcloud](https://twitter.com/darthcloud64)
 - [Additional protocol details for the Saturn](http://forums.modretro.com/index.php?threads/saturn-controller-protocol-mk80116-mk80100.11328/)

 ## Saturn 3D supported Game list, patent details, and more ##
https://segaretro.org/3D_Control_Pad
 
 ## Polling Oddity or Opportunity? ## 
The [Saturn only polls the 3D protocol at 16ms intervals](https://nfggames.com/forum2/index.php?topic=5055.0) (screen blank). There is however some suggestion in the below post that the Action Replay (this is firmware version specific) can attempt polling faster.  It's worth taking a look at as it may be possible to force games into a faster mode with patching.

Qoute:
"For whatever reason, it's only polling the controller for 14.25ms, with around 310us pauses in between, so it's polling much quicker, but the real issue is after this 14.25ms is up, it then drives the SEL line Hi and goes about it's business for ~2.5ms after that, then back to polling."

## Other demos and informational links ##
- [A DIY internal BlueRetro receiver for Saturn](https://twitter.com/nosIndulgences/status/1573719805496299520)
- [Hexfreq showing off a completely differnt bluetooth saturn solution](https://twitter.com/hexfreq/status/1468282054978818062)

## **Libraries in use** ##
- [ESP32-BLE-Gamepad](https://github.com/lemmingDev/ESP32-BLE-Gamepad)
 - This is subject to change long term as new and improved libraries emerge to support BLE HID gamepads

- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino)

## **Details on the Feather** ##
- If you want to remix the PCB's or understand more about the feather itself check out the following
- [Feather Pinouts and hardware overview](https://learn.adafruit.com/adafruit-huzzah32-esp32-feather/pinouts)
