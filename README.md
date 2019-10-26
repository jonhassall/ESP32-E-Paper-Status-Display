# ESP32-E-Paper-Status-Display
Use an ESP32 with E-Paper to make a status display gadget.

This code should let you make a regularly updating, low power status display, querying
a HTTPS API for JSON data, and displaying each heading and data in sequence.

A sample of the API data is provided [sample.json](https://raw.githubusercontent.com/jonhassall/ESP32-E-Paper-Status-Display/master/sample.json)

Set the URL and your WiFi credentials.

I used a T5s V2.1 ESP32 2.7" EPaper Plus Module but this code should work with similar modules.

Line 62 in [epdif.cpp](https://raw.githubusercontent.com/jonhassall/ESP32-E-Paper-Status-Display/master/epdif.cpp) may be of interest if you have trouble getting your E-Paper display to function:
`SPI.begin(SCK_PIN, -1, MOSI_PIN, CS_PIN);`
In example libraries, I found this line to be incorrect.