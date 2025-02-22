#include "FeedbackManager.h"
#include "ZaparooEsp32.hpp"
#include <FastLED.h>
#define UID_MAP_FILE "/uidExtdRecord.json"
#define FALL_BACK_DEF_IMG "/ZapEspBootLogo.jpg"
#define NUM_LEDS 8
#define DATA_PIN WS2812_DATA_PIN

FeedbackManager::FeedbackManager() {
    
}

FeedbackManager::~FeedbackManager() {
    
}

//Prefences has a 14 character limit for key
void FeedbackManager::init(Preferences* prefs, TFT_eSPI* tft) {
    preferences = prefs;
    tftScr = tft;
    audioGain = preferences->getFloat("audioGain", 21.0);
    motorEnabled = preferences->getBool("motor", false);
    launchLedEnabled = preferences->getBool("launchLed", false);
    audioEnabled = preferences->getBool("audio", false);
    pwrLedEnabled = preferences->getBool("pwrLed", false);
    resetOnRemove = preferences->getBool("resetOnRemove", true);
    buzzOnDetect = preferences->getBool("buzzOnDetect", true);
    buzzOnLaunch = preferences->getBool("buzzOnLaunch", true);
    buzzOnRemove = preferences->getBool("buzzOnRemove", true);
    buzzOnError = preferences->getBool("buzzOnError", true);
    defaultInsertAudio = preferences->getString("insertAudio", "");
    defaultLaunchAudio = preferences->getString("launchAudio", "");
    defaultRemoveAudio = preferences->getString("removeAudio", "");
    defaultErrorAudio = preferences->getString("errorAudio", "");
    defaultImgPath = preferences->getString("defImgPath", "");
    motorPin = preferences->getInt("motorPin", 32);
    setupPins();
    delay(500);
    createUidMappingFile();
    tftScr->begin();
    tftScr->begin();// Don't know why this has to be done twice but it works!!
    delay(500);
    dispDefaultImg(defaultImgPath);
}

void FeedbackManager::createUidMappingFile(){
  File uidFile;
  bool exists = true;
  
  if(!SD.exists(UID_MAP_FILE)){
    uidFile = SD.open(UID_MAP_FILE, FILE_WRITE);
    exists = false;
  }
  
  if(!exists){
    JsonDocument tmpDoc;
    tmpDoc["UID_ExtdRecs"][0]["UID"] = "";
    tmpDoc["UID_ExtdRecs"][0]["launchAudio"] = "";
    tmpDoc["UID_ExtdRecs"][0]["removeAudio"] = "";
    String tmpJson = "";
    serializeJson(tmpDoc, tmpJson);
    uidFile.print(tmpJson);
    uidFile.close();
  }
}


void FeedbackManager::setupPins() {
  if (motorEnabled) {
    pinMode(motorPin, OUTPUT);
  }
  if (pwrLedEnabled) {
    ledRingRed();
  }
  if (!SD.begin(BOARD_SD_CS)) {
    Serial.println(F("failed to do SD Card"));
  } 
}

void FeedbackManager::update(JsonDocument& doc) {
    serializeJson(doc, Serial);
    Serial.println();
    if (doc["data"].containsKey("audioGain")) {
        audioGain = doc["data"]["audioGain"].as<float>();
        preferences->putFloat("audioGain", audioGain);
    }
    if (doc["data"].containsKey("motorEnabled")) {
        motorEnabled = doc["data"]["motorEnabled"].as<bool>();
        preferences->putBool("motor", motorEnabled);
    }
    if (doc["data"].containsKey("launchLedEnabled")) {
        launchLedEnabled = doc["data"]["launchLedEnabled"].as<bool>();
        preferences->putBool("launchLed", launchLedEnabled);
    }
    if (doc["data"].containsKey("audioEnabled")) {
        audioEnabled = doc["data"]["audioEnabled"].as<bool>();
        preferences->putBool("audio", audioEnabled);
    }
    if (doc["data"].containsKey("pwrLedEnabled")) {
        pwrLedEnabled = doc["data"]["pwrLedEnabled"].as<bool>();
        preferences->putBool("pwrLed", pwrLedEnabled);
    }
    if (doc["data"].containsKey("resetOnRemove")) {
        resetOnRemove = doc["data"]["resetOnRemove"].as<bool>();
        preferences->putBool("resetOnRemove", resetOnRemove);
    }
    if (doc["data"].containsKey("buzzOnDetect")) {
        buzzOnDetect = doc["data"]["buzzOnDetect"].as<bool>();
        preferences->putBool("buzzOnDetect", buzzOnDetect);
    }
    if (doc["data"].containsKey("buzzOnLaunch")) {
        buzzOnLaunch = doc["data"]["buzzOnLaunch"].as<bool>();
        preferences->putBool("buzzOnLaunch", buzzOnLaunch);
    }
    if (doc["data"].containsKey("buzzOnRemove")) {
        buzzOnRemove = doc["data"]["buzzOnRemove"].as<bool>();
        preferences->putBool("buzzOnRemove", buzzOnRemove);
    }
    if (doc["data"].containsKey("buzzOnError")) {
        buzzOnError = doc["data"]["buzzOnError"].as<bool>();
        preferences->putBool("buzzOnError", buzzOnError);
    }
    if (doc["data"].containsKey("defaultInsertAudio")) {
        defaultInsertAudio = doc["data"]["defaultInsertAudio"].as<String>();
        preferences->putString("insertAudio", defaultInsertAudio);
    }
    if (doc["data"].containsKey("defaultLaunchAudio")) {
        defaultLaunchAudio = doc["data"]["defaultLaunchAudio"].as<String>();
        preferences->putString("launchAudio", defaultLaunchAudio);
        Serial.println(preferences->getString("launchAudio", "Not Found"));
    }
    if (doc["data"].containsKey("defaultRemoveAudio")) {
        defaultRemoveAudio = doc["data"]["defaultRemoveAudio"].as<String>();
        preferences->putString("removeAudio", defaultRemoveAudio);
    }
    if (doc["data"].containsKey("defaultErrorAudio")) {
        defaultErrorAudio = doc["data"]["defaultErrorAudio"].as<String>();
        preferences->putString("errorAudio", defaultErrorAudio);
    }
    if (doc["data"].containsKey("defaultImgPath")) {
        defaultImgPath = doc["data"]["defaultImgPath"].as<String>();
        preferences->putString("defImgPath", defaultImgPath);
    }
    // Pin assignments - saved to Preferences
    if (doc["data"].containsKey("motorPin")) {
        motorPin = doc["data"]["motorPin"].as<int>();
        preferences->putInt("motorPin", motorPin);
    }
}


void FeedbackManager::set(JsonDocument& doc) {
    doc["data"]["audioGain"] = audioGain;
    doc["data"]["motorEnabled"] = motorEnabled;
    doc["data"]["launchLedEnabled"] = launchLedEnabled;
    doc["data"]["audioEnabled"] = audioEnabled;
    doc["data"]["pwrLedEnabled"] = pwrLedEnabled;
    doc["data"]["resetOnRemove"] = resetOnRemove;
    doc["data"]["buzzOnDetect"] = buzzOnDetect;
    doc["data"]["buzzOnLaunch"] = buzzOnLaunch;
    doc["data"]["buzzOnRemove"] = buzzOnRemove;
    doc["data"]["buzzOnError"] = buzzOnError;
    doc["data"]["defaultInsertAudio"] = defaultInsertAudio;
    doc["data"]["defaultLaunchAudio"] = defaultLaunchAudio;
    doc["data"]["defaultRemoveAudio"] = defaultRemoveAudio;
    doc["data"]["defaultErrorAudio"] = defaultErrorAudio;
    doc["data"]["defaultImgPath"] = defaultImgPath;
    doc["data"]["motorPin"] = motorPin;
}


void FeedbackManager::motorOn(int predelay) {
    if (motorEnabled) {
        delay(predelay);
        analogWrite(motorPin, 255);
    }
}

void FeedbackManager::motorOff(int predelay) {
    if (motorEnabled) {
        delay(predelay);
        analogWrite(motorPin, 0);
    }
}

void FeedbackManager::launchLedOn(int predelay) {
    if (launchLedEnabled) {
        delay(predelay);
        ledRingGreen();
    }
}

void FeedbackManager::launchLedOff(int predelay, int postDelay) {
    if (launchLedEnabled) {
      delay(predelay);
      if (pwrLedEnabled) {
        ledRingRed();
      }else {
        ledRingOff();
      }
      delay(postDelay);
    }
}

void FeedbackManager::wifiLedOn() {
  CRGB leds[NUM_LEDS];
  FastLED.addLeds<WS2813, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(50);
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i] = CRGB::Blue;
    for(int j = i+1; j < NUM_LEDS; j++){
      leds[j] = CRGB::Red;
    }
    FastLED.show();
    delay(50);
  }    
}

void FeedbackManager::ledRingRed() {
    CRGB leds[NUM_LEDS];
    FastLED.addLeds<WS2813, DATA_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(50);
    for(int i = 0; i < NUM_LEDS; i++){
        leds[i] = CRGB::Red;
    }
    FastLED.show();
}

void FeedbackManager::ledRingGreen() {
    CRGB leds[NUM_LEDS];
    FastLED.addLeds<WS2813, DATA_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(50);
    for(int i = 0; i < NUM_LEDS; i++){
      leds[i] = CRGB::Green;
      for(int j = i+1; j < NUM_LEDS; j++){
        leds[j] = CRGB::Black;
      }
      FastLED.show();
      delay(50);
    }
    
}

void FeedbackManager::ledRingBlue() {
    CRGB leds[NUM_LEDS];
    FastLED.addLeds<WS2813, DATA_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(50);
    for(int i = 0; i < NUM_LEDS; i++){
        leds[i] = CRGB::Blue;
    }
    FastLED.show();
}

void FeedbackManager::ledRingOff() {
    CRGB leds[NUM_LEDS];
    FastLED.addLeds<WS2813, DATA_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(0);
    for(int i = 0; i < NUM_LEDS; i++){
        leds[i] = CRGB::Black;
    }
    FastLED.show();
}

void FeedbackManager::expressError(int code) {
    for (int i = 0; i < code; i++) {
        launchLedOn();
        if (buzzOnError) {
            motorOn(0);
            motorOff(800);
        }
        if (!defaultErrorAudio.isEmpty()) {
            playAudio(defaultErrorAudio.c_str());
        }
        launchLedOff(0, 400);
    }
}

void FeedbackManager::successActions(ZaparooToken* obj) {
    launchLedOn(0);
    const char* pathToPlay = obj->getLaunchAudio();
    if (pathToPlay == nullptr || strlen(pathToPlay) == 0) {
        pathToPlay = defaultLaunchAudio.c_str();
    }

    if (pathToPlay != nullptr && strlen(pathToPlay) > 0) {
        if (buzzOnLaunch) {
            motorOn(0);
            motorOff(100);
        }
        playAudio(pathToPlay);
    } else {
        if (buzzOnLaunch) {
            motorOn(0);
            motorOff(1000);
        }
    }
    launchLedOff(0, 0);
}

void FeedbackManager::setUidAudioMappings(ZaparooToken* obj) {
    File file;
    JsonDocument uidFile;
    if (SD.exists(UID_MAP_FILE)) {
        file = SD.open(UID_MAP_FILE, FILE_READ);
    } else {
        return;
    }
    String uid = String(obj->getId());
    while (file.available()) {
        DeserializationError error = deserializeJson(uidFile, file);
        if (!error) {
            for (JsonObject item : uidFile["UID_ExtdRecs"].as<JsonArray>()) {
                if (item["UID"] == uid) {
                    obj->setLaunchAudio(item["launchAudio"].as<String>().c_str());
                    obj->setRemoveAudio(item["removeAudio"].as<String>().c_str());
                    file.close();
                    return;
                }
            }
        }
    }
    file.close();
}

void FeedbackManager::getUidMappings(JsonDocument& toSet){
  JsonDocument parsed;
  File file;
  toSet["msgType"] = "getUIDExtdRec";
  if(SD.exists(UID_MAP_FILE)){
    file = SD.open(UID_MAP_FILE, FILE_READ);
  }
  while (file.available()) { 
    DeserializationError error = deserializeJson(parsed, file);
    if(!error){
      toSet["data"] = parsed;
      file.close();
      return;
    }
  }
  file.close();
  return;
}

void FeedbackManager::saveUidMapping(JsonDocument &value){
  File file;
  if(SD.exists(UID_MAP_FILE)){
    file = SD.open(UID_MAP_FILE, FILE_WRITE);
  }
  while (file.available()) {
    serializeJson(value, file);
    file.close();
    return;
  }
  file.close();
  return;
}

int FeedbackManager::playAudio(const char* audioPath) {
    if (!audioPath || strlen(audioPath) == 0) {
        delay(1000);
        return 0;
    }
    Audio audio;  
    audio.setPinout(BOARD_VOICE_BCLK, BOARD_VOICE_LRCLK, BOARD_VOICE_DIN);
    audio.setVolume(audioGain);
    audio.connecttoFS(SD, audioPath);
    audio.loop();
    while(audio.isRunning()){
      audio.loop();
    }
    return 0;
}



void FeedbackManager::cardInsertedActions(ZaparooToken* obj) {
    const char* pathToPlay = obj->getDetectAudio();
    
    if (!pathToPlay || strlen(pathToPlay) == 0) {
        pathToPlay = defaultInsertAudio.c_str();
    }
    if (pathToPlay && strlen(pathToPlay) > 0) {
        playAudio(pathToPlay);
    }
    if(obj->isLaunchJPEGSet()){
      const char* imgToShow = obj->getLaunchJPEG();
      if (imgToShow || strlen(imgToShow) > 0) {
        Serial.println(imgToShow);
        dispJpgImg(imgToShow);
      }
    }
    if (buzzOnDetect) {
        motorOn(0);
        motorOff(100);
    }
}

void FeedbackManager::cardRemovedActions(ZaparooToken* obj) {
    const char* pathToPlay = obj->getRemoveAudio();
    if (!pathToPlay || strlen(pathToPlay) == 0) {
        pathToPlay = defaultRemoveAudio.c_str();
    }
    if (pathToPlay && strlen(pathToPlay) > 0) {
        playAudio(pathToPlay);
    }
    if (buzzOnRemove) {
        motorOff();
    }
    dispDefaultImg(defaultImgPath);
}

void FeedbackManager::dispDefaultImg(String imgPath){
  const char* imgToShow = imgPath.c_str();
  if (imgToShow == nullptr || strlen(imgToShow) == 0) {
    imgToShow = FALL_BACK_DEF_IMG;
  }
  if (SD.exists(imgToShow)) {
    tftScr->setRotation(0);
    tftScr->fillScreen(TFT_WHITE);
    drawSdJpeg(imgToShow, 0, 0);
  } else {
    return;
  }  
}

void FeedbackManager::dispJpgImg(const char *imgPath){
  //const char* imgToShow = imgPath.c_str();
  if (imgPath == nullptr || strlen(imgPath) == 0) {
    return;
  }
  if (SD.exists(imgPath)) {
    tftScr->fillScreen(TFT_WHITE);
    drawSdJpeg(imgPath, 0, 0);
  } else {
    return;
  }
}

void FeedbackManager::drawSdJpeg(const char *filename, int xpos, int ypos) {
  // Open the named file (the Jpeg decoder library will close it)
  File jpegFile = SD.open(filename, FILE_READ);  // or, file handle reference for SD library 
  if ( !jpegFile ) {
    Serial.print("ERROR: File \""); Serial.print(filename); Serial.println ("\" not found!");
    return;
  }
  bool decoded = JpegDec.decodeSdFile(jpegFile);  // Pass the SD file handle to the decoder,
  if (decoded) {
    // render the image onto the screen at given coordinates
    jpegRender(xpos, ypos);
  }
  else {
    Serial.println("Jpeg file format not supported!");
  }
}

void FeedbackManager::jpegRender(int xpos, int ypos) {

  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;

  bool swapBytes = tftScr->getSwapBytes();
  tftScr->setSwapBytes(true);  
  // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
  // Typically these MCUs are 16x16 pixel blocks
  // Determine the width and height of the right and bottom edge image blocks
  uint32_t min_w = jpg_min(mcu_w, max_x % mcu_w);
  uint32_t min_h = jpg_min(mcu_h, max_y % mcu_h);
  // save the current image block size
  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;
  // record the current time so we can measure how long it takes to draw an image
  uint32_t drawTime = millis();
  // save the coordinate of the right and bottom edges to assist image cropping
  // to the screen size
  max_x += xpos;
  max_y += ypos;
  // Fetch data from the file, decode and display
  while (JpegDec.read()) {    // While there is more data in the file
    pImg = JpegDec.pImage ;   // Decode a MCU (Minimum Coding Unit, typically a 8x8 or 16x16 pixel block)
    // Calculate coordinates of top left corner of current MCU
    int mcu_x = JpegDec.MCUx * mcu_w + xpos;
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;
    // check if the image block size needs to be changed for the right edge
    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;
    // check if the image block size needs to be changed for the bottom edge
    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;
    // copy pixels into a contiguous block
    if (win_w != mcu_w)
    {
      uint16_t *cImg;
      int p = 0;
      cImg = pImg + win_w;
      for (int h = 1; h < win_h; h++)
      {
        p += mcu_w;
        for (int w = 0; w < win_w; w++)
        {
          *cImg = *(pImg + w + p);
          cImg++;
        }
      }
    }
    // calculate how many pixels must be drawn
    uint32_t mcu_pixels = win_w * win_h;
    // draw image MCU block only if it will fit on the screen
    if (( mcu_x + win_w ) <= tftScr->width() && ( mcu_y + win_h ) <= tftScr->height())
      tftScr->pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
    else if ( (mcu_y + win_h) >= tftScr->height())
      JpegDec.abort(); // Image has run off bottom of screen so abort decoding
  }
  tftScr->setSwapBytes(swapBytes);
}

