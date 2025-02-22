#pragma once
#include "Audio.h"
#include <LittleFS.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "ZaparooToken.h"
#include <FastLED.h>
#include <TFT_eSPI.h>
#include <JPEGDecoder.h>

class FeedbackManager {
private:
    Preferences* preferences;
    TFT_eSPI* tftScr;
    void setupPins();
    void createUidMappingFile();
    void setDefaultImage(string imgPath);
public:
    float audioGain = 21.0;
    bool motorEnabled = false;
    bool launchLedEnabled = true;
    bool audioEnabled = false;
    bool pwrLedEnabled = true;
    bool resetOnRemove = true;
    bool buzzOnDetect = true;
    bool buzzOnLaunch = true;
    bool buzzOnRemove = true;
    bool buzzOnError = true;

    String defaultInsertAudio = "";
    String defaultLaunchAudio = "";
    String defaultRemoveAudio = "";
    String defaultErrorAudio = "";
    String defaultImgPath = "";

    int motorPin = -1;

    FeedbackManager();
    ~FeedbackManager();
    void init(Preferences* prefs, TFT_eSPI* tftScr);
    void update(JsonDocument& doc);
    void set(JsonDocument& doc);
    void motorOn(int predelay = 0);
    void motorOff(int predelay = 0);
    void launchLedOn(int predelay = 0);
    void launchLedOff(int predelay = 0, int postDelay = 0);
    void wifiLedOn();
    void expressError(int code);
    void successActions(ZaparooToken* obj);
    void setUidAudioMappings(ZaparooToken* obj);
    void getUidMappings(JsonDocument& toSet);
    void saveUidMapping(JsonDocument& value);
    int playAudio(const char* audioPath);
    void cardInsertedActions(ZaparooToken* obj);
    void cardRemovedActions(ZaparooToken* obj);
    void ledRingRed();
    void ledRingBlue();
    void ledRingGreen();
    void jpegRender(int xpos, int ypos);
    void drawSdJpeg(const char *filename, int xpos, int ypos);
    void dispDefaultImg(String imgPath);
    void dispJpgImg(const char *imgPath);
    void ledRingOff();
};
