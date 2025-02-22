#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <LittleFS.h>
#include <ESPWebFileManager.h>
#include <NfcAdapter.h>
#include <ESPmDNS.h>
#include <ZaparooLaunchApi.h>
#include <ZaparooRestResult.h>
#include "index.h"
#include "ZaparooEsp32.hpp"
#include "ZaparooScanner.cpp"
#include "FeedbackManager.h"
#include "scanners/ZaparooPN532Scanner.cpp"
#include <TFT_eSPI.h>
#include <JPEGDecoder.h>

//NFC
PN532_I2C pn532_i2c(Wire);

TFT_eSPI tft = TFT_eSPI();

//Common Setup
Preferences preferences;
AsyncWebServer server(80);
AsyncWebSocket ws1("/ws");
ESPWebFileManager fileManager(FS_SD, false, false);
ZaparooLaunchApi ZapClient;
ZaparooScanner* tokenScanner = NULL;
FeedbackManager feedback;

//globals
String ZAP_URL = "ws://<replace>:7497" + String(ZaparooLaunchApi::wsPath);
ZaparooToken* token = NULL;
bool inserted = false;
bool isPN532 = false;
int timeoutLoop = 0;
bool isConnected = false;
bool isWebLog = false;
bool softReset = false;
bool misterEnabled = true;
bool steamEnabled = false;
bool uidScanMode= false;
bool serialOnly = false;
String steamIp = "steamOS.local";
String zapIp = "mister.local";
String lastSerialCommand = "";

//Prototypes
void notifyClients(const String& txtMsgToSend, const String& msgType);
void handleWebSocketMessage(void* arg, uint8_t* data, size_t len);


void setPref_Bool(const String& key, bool valBool) {
  preferences.putBool(key.c_str(), valBool);
}

void setPref_Int(const String& key, int valInt) {
  preferences.putInt(key.c_str(), valInt);
}

void setPref_Str(const String& key, const String& valStr) {
  preferences.putString(key.c_str(), valStr);
}

void setPref_Float(const String& key, float valFloat) {
  preferences.putFloat(key.c_str(), valFloat);
}

void notifyClients(const String& txtMsgToSend, const String& msgType) {
  if(!serialOnly && txtMsgToSend != "KeepAlive") Serial.println(txtMsgToSend);
  if (isWebLog) {
    JsonDocument msgJson;
    msgJson["msgType"] = "notify";
    msgJson["data"]["msgTxt"] = txtMsgToSend;
    msgJson["data"]["type"] = msgType;
    String output;
    serializeJson(msgJson, output);
    delay(200);
    ws1.textAll(output.c_str());
  }
}

void cmdClients(JsonDocument& cmdJson) {
  String output;
  serializeJson(cmdJson, output);
  Serial.print("WS MSG SENT: ");
  Serial.println(output);
  delay(100);
  ws1.textAll(output.c_str());
}

void onEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type,
             void* arg, uint8_t* data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      isWebLog = true;
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      setPref_Bool("enNfcWr", false);
      isWebLog = false;
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws1.onEvent(onEvent);
  server.addHandler(&ws1);
}

void startApMode() {
  server.end();
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  WiFi.softAP("zapesp32", "zapesp32");
  Serial.println("Starting AP Mode:");
  Serial.println(WiFi.softAPIP());
  server.begin();
  initWebSocket();
  MDNS.begin("zapesp");
  Serial.flush();
  delay(100);
}

void connectWifi() {
  int i_strongest = -1;
  int32_t rssi_strongest = -100;
  if (WiFi.status() == WL_CONNECTED || WiFi.getMode() == WIFI_MODE_AP) {
    return;
  }
  WiFi.disconnect();
  MDNS.end();
  String ssid = preferences.getString("wifiSSID", "");
  if(ssid.isEmpty()){
    startApMode();
    return;
  }
  //Code to connect esp32 to strongest AP in mesh network env
  int n = WiFi.scanNetworks();
  if (n != 0) {
    for (int i = 0; i < n; ++i) {
      if ((ssid == String(WiFi.SSID(i)) && (WiFi.RSSI(i)) > rssi_strongest)) {
        rssi_strongest = WiFi.RSSI(i);
        i_strongest = i;
      }
    }
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, preferences.getString("wifiPass", ""), 0, WiFi.BSSID(i_strongest));
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, preferences.getString("wifiPass", ""));
  }
  
  int retries = 30;
  while (WiFi.status() != WL_CONNECTED && retries--) {
    feedback.wifiLedOn();
    Serial.print(".");
  }
  if (WiFi.status() != WL_CONNECTED) {
    startApMode();
    return;
  }
  // Initialize mDNS
  retries = 10;
  while (!MDNS.begin("zapesp") && retries--) {
    Serial.println("Error setting up MDNS responder!");
    delay(1000);
  }
  Serial.println("mDNS started - access the web UI @ http://zapesp.local");
  Serial.print("WiFi connected - Zap ESP IP = ");
  Serial.println(WiFi.localIP());
  WiFi.setSleep(false);
  server.begin();
  initWebSocket();
  feedback.motorOn();
  feedback.wifiLedOn();
  feedback.motorOff(250);
  feedback.motorOn(100);
  feedback.wifiLedOn();
  feedback.motorOff(250);
  if(feedback.pwrLedEnabled){
    feedback.ledRingRed();
  } else {
    feedback.ledRingOff();
  }
  
}

void writeTagLaunch(String& launchCmd, String& audioLaunchFile, String& audioRemoveFile, String& launchJPEGFile) {
  String tmpLaunchCmd = launchCmd;
  JsonDocument cmdData;
  cmdData["msgType"] = "writeResults";
  tmpLaunchCmd.replace("launch_cmd::", "");
  notifyClients("Launch Cmd Written: " + launchCmd, "log");
  if (tokenScanner->tokenPresent()) {
    bool success = tokenScanner->writeLaunch(launchCmd, audioLaunchFile, audioRemoveFile, launchJPEGFile);
    if (success) {
      cmdData["data"]["isSuccess"] = true;
      cmdData["data"]["isCardDetected"] = true;
      cmdClients(cmdData);
    } else {
      cmdData["data"]["isSuccess"] = false;
      cmdData["data"]["isCardDetected"] = true;
      cmdClients(cmdData);
    }
  } else {
    cmdData["data"]["isSuccess"] = false;
    cmdData["data"]["isCardDetected"] = false;
    cmdClients(cmdData);
  }  
  tokenScanner->halt();
}

//Load the UIDExtdRec.json and pass to Web Client
void getUIDExtdRec(){
  JsonDocument result;
  feedback.getUidMappings(result);
  cmdClients(result);
}


bool send(String& gamePath) {
  String message;
  bool sent = false;
  if (serialOnly) {
    lastSerialCommand = "SCAN\ttext=" + gamePath;
    if(!feedback.resetOnRemove){
      lastSerialCommand = lastSerialCommand + "\tremovable=no";
    }
    Serial.println(lastSerialCommand);
    Serial.flush();
    message = "Sent game path to serial: " + gamePath;
    sent = true;
  } else {
    String newURL = ZAP_URL;
    newURL.replace("<replace>", gamePath.startsWith("steam://") ? steamIp : zapIp);
    ZapClient.url(newURL);
    notifyClients("URL: " + newURL, "log");
    int code = ZapClient.launch(gamePath);
    if (code > 0) {
      feedback.expressError(code);
      message = "Zaparoo Error Launching Game: " + gamePath + " | Error Code: " + code;
    } else {
      message = "Launched Game: " + gamePath;
      sent = true;
    }
  }
  notifyClients(message, "log");
  return sent;
}

bool sendUid(String& uid) {
  String message;
  bool sent = false;
  if (serialOnly) {
    lastSerialCommand = "SCAN\tuid=" + uid;
    if(!feedback.resetOnRemove){
      lastSerialCommand = lastSerialCommand + "\tremovable=no";
    }
    Serial.println(lastSerialCommand);
    Serial.flush();
    message = "Sent Card/Tag UID: " + uid;
    sent = true;
  } else {
    //not possible to determine if steam game from UID so always default to MiSTer if enabled
    String newURL = ZAP_URL;
    newURL.replace("<replace>", misterEnabled ? zapIp : steamIp);
    ZapClient.url(newURL);
    int code = ZapClient.launchUid(uid);
    if (code > 0) {
      feedback.expressError(code);
      message = "Zaparoo Error Sending Card/Tag UID " + uid + " | Error Code: " + code;
    } else {
      message = "Sent Card/Tag UID: " + uid;
      sent = true;
    }
  }
  notifyClients(message, "log");
  return sent;
}

//Send the detected UID to Web Client for Audio Mapping
void sendUIDtoWeb(String UIDStr){
  JsonDocument UIDData;
  UIDData["msgType"] = "UIDTokenID";
  UIDData["data"] = UIDStr;
  cmdClients(UIDData);
}

void getWebConfigData() {
  notifyClients("Retrieving Current Zap ESP Config Data", "log");
  JsonDocument configData;
  feedback.set(configData);
  configData["msgType"] = "ConfigData";
  configData["data"]["zapIp"] = preferences.getString("zapIp", "mister.local");
  configData["data"]["misterEnabled"] = preferences.getBool("misterEnabled", true);
  configData["data"]["steamEnabled"] = preferences.getBool("steamEnabled", false);
  configData["data"]["serialOnly"] = preferences.getBool("serialOnly", false);
  configData["data"]["steamIp"] = preferences.getString("steamIp", "steamOS.local");
  configData["data"]["PN532_module"] = isPN532;
  configData["data"]["zapWsPath"] = ZaparooLaunchApi::wsPath; 
  cmdClients(configData);
}

void setWebConfigData(JsonDocument& cfgData) {
  notifyClients("ZAP ESP Now Saving Config Data", "log");
  feedback.launchLedOff();
  feedback.update(cfgData);
  if(cfgData["data"].containsKey("zapIp")){
    setPref_Str("zapIp", cfgData["data"]["zapIp"]);
  }
  if(cfgData["data"].containsKey("steamEnabled")){
    setPref_Bool("steamEnabled", cfgData["data"]["steamEnabled"]);
  }
  if(cfgData["data"].containsKey("misterEnabled")){
    setPref_Bool("misterEnabled", cfgData["data"]["misterEnabled"]);
  }
  if(cfgData["data"].containsKey("serialOnly")){
    setPref_Bool("serialOnly", cfgData["data"]["serialOnly"]);
  }
  if(cfgData["data"].containsKey("steamIp")){
    setPref_Str("steamIp", cfgData["data"]["steamIp"]);
  }
  preferences.end();
  Serial.println("Rebooting after saving");
  ESP.restart();
}

void handleResetRequest(){
  JsonDocument cmdData;
  cmdData["msgType"] = "closeWS";
  cmdClients(cmdData);
  delay(1000);
  ws1.closeAll();
  ws1.cleanupClients();
  ESP.restart();
}

void handleWebSocketMessage(void* arg, uint8_t* data, size_t len) {
  AwsFrameInfo* info = static_cast<AwsFrameInfo*>(arg);
  if (!info->final || info->index != 0 || info->len != len || info->opcode != WS_TEXT) {
    return;  // Invalid frame
  }
  String webCmd(reinterpret_cast<char*>(data), len);
  JsonDocument root;
  DeserializationError error = deserializeJson(root, webCmd);
  if (error) {
    notifyClients("Failed to Parse JSON", "log");
    return;
  }
  String command = root["cmd"].as<String>();

  if (command == "set_WriteMode") {
    bool enableWriteMode = root["data"];
    notifyClients(enableWriteMode ? "NFC Tag Write Mode Enabled" : "NFC Tag Write Mode Disabled", "log");
    setPref_Bool("enNfcWr", enableWriteMode);
  } else if (command == "write_Tag_Launch_Game" && preferences.getBool("enNfcWr", false)) {
      notifyClients("NFC Tag Writing the Launch Game Command", "log");
      String launchData = root["data"]["launchData"].as<String>();
      String audioLaunchPath = root["data"]["audioLaunchPath"].as<String>();
      String audioRemovePath = root["data"]["audioRemovePath"].as<String>();
      String launchJPEGPath = root["data"]["launchJPEGPath"].as<String>();
      writeTagLaunch(launchData, audioLaunchPath, audioRemovePath, launchJPEGPath);
  } else if (command == "get_Current_Config") {
      getWebConfigData();
  } else if (command == "set_Current_Config") {
      setWebConfigData(root);
  } else if (command == "Test_Tag_Launch_Game") {
      notifyClients("Test Launching Game", "alert");
      String data = root["data"].as<String>();
      send(data);
  } else if (command == "closeWS") {
      setPref_Bool("enNfcWr", false);
      ws1.closeAll();
      ws1.cleanupClients();
  } else if (command == "getUIDExtdRec") {
      notifyClients("Retrieving UIDExtdRec Data", "log");
      getUIDExtdRec();
  } else if (command == "set_UIDMode") {
      bool enableUIDMode = root["data"];
      notifyClients(enableUIDMode ? "UID Scanning Mode Enabled" : "UID Scanning Mode Disabled", "alert");
      uidScanMode= enableUIDMode;
      //get UIDExtdRec data if enabling UID mode
      if(enableUIDMode){
        getUIDExtdRec();
      }
  } else if (command == "saveUIDExtdRec") {
      notifyClients("Saving UIDExtdRec Data", "log");
      JsonDocument data = root["data"];
      feedback.saveUidMapping(data);
      //saveUIDExtdRec(root["data"]);
  } else if (command == "wifi") {
    setPref_Str("wifiSSID", root["data"]["ssid"].as<String>());
    setPref_Str("wifiPass", root["data"]["password"].as<String>());
    WiFi.disconnect(); 
    WiFi.mode(WIFI_STA);
    notifyClients("Updated ssid", "log");
  } else if (command == "doReset") {
      handleResetRequest();
  } else if (command == "ping") {
      notifyClients("KeepAlive", "log");
  } else {
    notifyClients("Unknown Command", "log");
  }
}

void handleSend(){
  bool sent = false;
  bool playAudioFirst = serialOnly && feedback.resetOnRemove && uidScanMode;
  feedback.setUidAudioMappings(token);
  if(playAudioFirst){
    feedback.successActions(token); //Play the audio before launch to support remove with simple serial
    sent = true;
  }
  if (!token->isBlankPayload() && !uidScanMode) {
    String payload = String(token->getPayload());
    sent = send(payload);
  } else if (token->isIdSet() && !uidScanMode) {
    String id = String(token->getId());
    sent = sendUid(id);
  }else if(token->isIdSet() && uidScanMode){
    String id = String(token->getId());
    sendUIDtoWeb(id);
  }
  if(!playAudioFirst && sent){
    feedback.successActions(token);
  }
}

//Loop 20 times per read to break out and run other loop code
bool readScanner() {
  for (int i = 0; i < 20 && !preferences.getBool("enNfcWr", false); i++) {
    bool present = tokenScanner->tokenPresent();
    ZaparooToken* parsed = present ? tokenScanner->getNewToken() : NULL;
    if (present && parsed) {
      if (!parsed->getValid()) {
        inserted = false;
        delete parsed;
        delay(10);
        continue;
      }
      delete token;
      token = NULL;
      token = parsed;
      feedback.cardInsertedActions(token);
      inserted = true;
      handleSend();
    } else if (!present && inserted) {  //Must have been removed
      String removeAudio = "";
      if (token->isRemoveAudioSet()) {
        removeAudio = token->getRemoveAudio();
      }
      feedback.cardRemovedActions(token);
      if (feedback.resetOnRemove && !serialOnly && token->isPayloadSet()) {
        String payloadAsString = String(token->getPayload());
        if (!payloadAsString.startsWith("steam://")) {
          ZapClient.stop();
        }
      }
      notifyClients("Tag Removed", "log");
      inserted = false;
      tokenScanner->halt();
      return true;
    }else if (present && inserted && serialOnly && feedback.resetOnRemove){
      Serial.println(lastSerialCommand);
      Serial.flush();
    }
    delay(10);
  }
  return false;
}

void setup() {
  //all SPI pins must be pulled high to avoid issues
  pinMode(DISPLAY_CS, OUTPUT);
  digitalWrite(DISPLAY_CS, HIGH);
  pinMode(BOARD_SD_CS, OUTPUT);
  digitalWrite(BOARD_SD_CS, HIGH);
  pinMode(BOARD_LORA_CS, OUTPUT);
  digitalWrite(BOARD_LORA_CS, HIGH);
  pinMode(BOARD_PWR_EN, OUTPUT);
  digitalWrite(BOARD_PWR_EN, HIGH);
  //
  Serial.begin(115200);
  SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);
  Wire.setPins(BOARD_I2C_SDA, BOARD_I2C_SCL);
  Wire.begin();
  ZaparooPN532Scanner* pnScanner = new ZaparooPN532Scanner();
  pnScanner->setConfig(pn532_i2c);
  pnScanner->setResetPin(BOARD_PN532_RF_REST);
  tokenScanner = pnScanner;
  isPN532 = true;
  //Check for PN532 Card Initalisation Failure and reset if in error
  if(!tokenScanner->init() && isPN532){
    tokenScanner->reset();
  }
  preferences.begin("qrplay", false);
  feedback.init(&preferences, &tft);
  setPref_Bool("enNfcWr", false);
  uidScanMode= false;
  
  //set globals to reduce the number of call to preference library (performance)
  zapIp = preferences.getString("zapIp", "mister.local");
  misterEnabled = preferences.getBool("misterEnabled", true);
  steamEnabled = preferences.getBool("steamEnabled", false);
  steamIp = preferences.getString("steamIp", "steamOS.local");
  serialOnly = preferences.getBool("serialOnly", false);
  
  //init WebFileManager
  if (!fileManager.begin()) {
    Serial.println("Failed to find SD Card");
  }
  fileManager.setServer(&server);
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse_P(200, "text/html", index_html, index_html_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  
}


void loop() {
  connectWifi();
  if (!preferences.getBool("enNfcWr", false)) {
    readScanner();
  }
  delay(50);
}
