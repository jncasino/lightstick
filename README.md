# Movie Prop Controller

You will need the following:

-ESP32, pretty much any you can find one on a retail store, but preferably the Seeed Studio XIAO ESP32C3 or other small one that can fit inside a stick.  
-Power Source, currenly this one runs off a battery bank but anything that can power the ESP32 + a power budget of ~5W for LEDs
-NeoPixel Ring LED or other LED that can be controlled by ESP32
- A 3D printed stick of choice with space to hide the electronics. 
## Step 1. 
Load the Arduino sketch into the IDE and connect your ESP32. Modify the Wi-Fi Credentials with yours, set the number of LEDs and upload the sketch.
        - Make sure you set the number of LEDs in NUM_LED to however many you have. 

## Step 2. 
Connect the 5V, Ground, and data pins and make sure they matched the output in the sketch.
## Step 3. 
Take note of the IP address in the Serial Output Monitor.
## Step 3.
Open the HTML file in any browser or on an Android phone and enter the IP address and select Test.
## Step 4.
Import any of the csv sequence files here or make your own in the Web App and export it as you wish.
## Step 5.
Under Playback Test, hit play!
