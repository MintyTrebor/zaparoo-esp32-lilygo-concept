# ZapEsp32

The goal of this project is to launch games via the [Zaparoo Service](https://wiki.zaparoo.org/Main_Page) on Mister/Steam over Wifi, with additional audio & rumble features. It also supports launching games via [Simple Serial](https://wiki.zaparoo.org/Reader_Drivers#Simple_Serial).

## Features:  
- Supports Universal launching for both MiSTer and Steam
- Play Audio files for Card Detected, Game Launched, and Card Removed events.  
- Play game specific Audio Files for Game Launch and Card Removed events
- SD Card Module support (Only Supported with PN532 Module installed).
- A Web UI to
  - configure the ESP32's LED's, Rumble Motor, & Audio Module's GPIO pins (if installed),
  - change Zaparoo MiSTer and Steam connection settings.
  - upload audio files to SD Card (if enabled), or ESP32 SPIFFS partition.
  - Search for games & write NFC tag/Cards with extended information.
  - Set default detect, launch, error, and remove events audio files
  - Configure unique launch audio and card removal audio sounds per game/card.  
  - Create Zaparoo Mapping DB entries to launch games from read-only NFC Tags/Amiibo's.  
  - Set launch & remove event audio clip files for readonly RFID/NFC tags (eg amiibo)


 Please refer to the [wiki](https://github.com/MintyTrebor/zaparoo-esp32/wiki) for full info and build details
