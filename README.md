# CO2
CO2 Monitoring System

This is an experimental sketch that I wrote for my CO2 Monitoring System. it runs on an ESP32 based development board (LOLIN32) and reads data from a CCS811/HDC1080 breakout board over I2C As well as an MH-Z19B sensor via UART through the SoftwareSerial Library. 

For this to work, you will need to patch the PubSubClient library, specifically the header file and adjust MQTT_MAX_PACKET_SIZE to 256.

Feel free to make any changes or improve on this sketch.
