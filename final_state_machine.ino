#include <WiFi.h> //Connect to WiFi Network
#include <Wire.h>
#include <esp_now.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h> //Used in support of TFT Display
#include <ArduCAM.h>
#include "memorysaver.h"
#include <string.h>  //used for some string handling and processing.
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <esp_wifi.h>
//#include <Arduino.h>
#include <DFRobotDFPlayerMini.h> // MP3 Player
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

// Sets up MP3
HardwareSerial mySoftwareSerial(1);
DFRobotDFPlayerMini myDFPlayer;
void printDetail(uint8_t type, int value);

/* CONSTANTS */
//Prefix to POST request:
const char PREFIX[] = "{\"wifiAccessPoints\": ["; //beginning of json body
const char SUFFIX[] = "]}"; //suffix to POST request
const char API_KEY[] = // redacted for security
const int MAX_APS = 5;


//for audio recording 
const int DELAY = 1000;
const int SAMPLE_FREQ = 8000;                          // Hz, telephone sample rate
const int SAMPLE_DURATION = 5;                        // duration of fixed sampling (seconds)
const int NUM_SAMPLES = SAMPLE_FREQ * SAMPLE_DURATION;  // number of of samples
const int ENC_LEN = (NUM_SAMPLES + 2 - ((NUM_SAMPLES + 2) % 3)) / 3 * 4;  // Encoded length of clip

/* CONSTANTS */
//Prefix to POST request:
const char audioPREFIX[] = "{\"config\":{\"encoding\":\"MULAW\",\"sampleRateHertz\":8000,\"languageCode\": \"en-US\", \"speechContexts\":[{\"phrases\":[],\"boost\": 2}]}, \"audio\": {\"content\":\"";
const char audioSUFFIX[] = "\"}}"; //suffix to POST request
const int AUDIO_IN = 36; //pin where microphone is connected - originally 36
char speech_data[ENC_LEN + 200] = {0}; //global used for collecting speech data
const char*  audioSERVER = "speech.google.com";  // Server URL
char finalword[100];
char body[1000]; //for body



const int BUTTON1 = 14; //switch mode //orign 14
const int BUTTON2 = 39; //media mode
const int BUTTON3 = 38; //communication mode
const int BUTTON4 = 34; //reset mode
 
const int BASE = 5;
const int MEDIA = 1;
const int NAVIGATION = 2;

const int NEXT = 1;
const int PREV = 2;
const int PAUSE = 4;
const int PLAY = 3;
const int RESET = 5;

bool playingMedia = false;
bool pausingMedia = true;
bool nextingMedia = false;
bool previngMedia = false;



uint8_t broadcastAddress[] = {0x8C, 0xAA, 0xB5, 0xB5, 0xAD, 0x90};

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
  int gesture;
} struct_message;

// Create a struct_message called myData
struct_message myData;

esp_now_peer_info_t peerInfo;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}


 
//states for doing it on the release
bool prevStateButton1 = false;
uint32_t timerdot = 0;
 
// Filenames to mp3 content (number represent mp3's name)
const int COMMUNICATION_CALLOUT = 1;
const int MEDIA_CALLOUT = 2;
const int SELECT_MODE_CALLOUT = 3;
const int NAVIGATION_CALLOUT = 4;
const int MIN_SONG = 5;
const int MAX_SONG = 9;

int curr_song = MIN_SONG;
 
uint8_t state = NAVIGATION;  //system state
uint8_t returnval;
uint8_t oldstate;
uint8_t button_state; //used for containing button state and detecting edges
int old_button_state = 1; //used for detecting button edges
uint32_t time_since_sample;      // used for microsecond timing


int wifi_object_builder(char* object_string, uint32_t os_len, uint8_t channel, int signal_strength, uint8_t* mac_address) {
    char tmp[90];
  
    int tmplength = sprintf(tmp, "{\"macAddress\": \"%02x:%02x:%02x:%02x:%02x:%02x\", \"signalStrength\": %d, \"age\": 0, \"channel\": %d}", 
    mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5], signal_strength, channel);
    
    if (tmplength < os_len) {
      int length = sprintf(object_string, "{\"macAddress\": \"%02x:%02x:%02x:%02x:%02x:%02x\", \"signalStrength\": %d, \"age\": 0, \"channel\": %d}", 
    mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5], signal_strength, channel);
      return length;
    }
    return 0;
}

char*  SERVER = "googleapis.com";  // Server URL
 
#if !(defined ESP32)
#error Please select the ArduCAM ESP32 UNO board in the Tools/Board
#endif
//This demo can only work on OV2640_MINI_2MP or ARDUCAM_SHIELD_V2 platform.
#if !(defined(OV2640_MINI_2MP) || defined(OV5640_MINI_5MP_PLUS) || defined(OV5642_MINI_5MP_PLUS) \
      || defined(OV5642_MINI_5MP) || defined(OV5642_MINI_5MP_BIT_ROTATION_FIXED) \
      || (defined(ARDUCAM_SHIELD_V2) && (defined(OV2640_CAM) || defined(OV5640_CAM) || defined(OV5642_CAM))))
#error Please select the hardware platform and camera module in the ../libraries/ArduCAM/memorysaver.h file
#endif
 
// set GPIO17 as the slave select :
const int CS = 25; // 34 for esps32
const int CAM_POWER_ON = 10;
#if defined(OV2640_MINI_2MP) || defined(OV2640_CAM)
ArduCAM myCAM(OV2640, CS);
#elif defined(OV5640_MINI_5MP_PLUS) || defined(OV5640_CAM)
ArduCAM myCAM(OV5640, CS);
#elif defined(OV5642_MINI_5MP_PLUS) || defined(OV5642_MINI_5MP) || defined(OV5642_MINI_5MP_BIT_ROTATION_FIXED) || (defined(OV5642_CAM))
ArduCAM myCAM(OV5642, CS);
#endif
 
const uint16_t RESPONSE_TIMEOUT = 6000;
const uint16_t IN_BUFFER_SIZE = 9000; //size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 3000; //size of buffer to hold HTTP response
char request[5000];
char request2[IN_BUFFER_SIZE];
char newmode[5];
char getresponse[3];

const uint16_t JSON_BODY_SIZE = 4000;
char response[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request
// char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char json_body[JSON_BODY_SIZE];
 
//you can change the value of wifiType to select Station or AP mode.
//Default is AP mode.
int wifiType = 1;  // 0:Station  1:AP
 
//AP mode configuration
//Default is arducam_esp8266.If you want,you can change the AP_aaid  to your favorite name
const char *AP_ssid = "arducam_esp32";
//Default is no password.If you want to set password,put your password here
const char *AP_password = NULL;

char network[] = "EECS_Labs";
char passdecodedWord[] = "";

const char *ssid = // redacted for security
const char *password = // redacted for security
 
 
static const size_t bufferSize = 5000;
static uint8_t buffer[bufferSize] = { 0xFF };
 
uint8_t temp = 0, temp_last = 0;
int i = 0;
int image_index = 0;
bool is_header = false;
 
//ESP32WebServer server(80);
WiFiClientSecure client; //global WiFiClient Secure object
WiFiClient client2;
 
//File file;
 
void start_capture() {
  myCAM.clear_fifo_flag();
  myCAM.start_capture();
}
 
void readImage() {
  uint32_t len = myCAM.read_fifo_length();

  if (len >= MAX_FIFO_SIZE)  //8M
  {
    Serial.println(F("Over size."));
  }
  if (len == 0)  //0 kb
  {
    Serial.println(F("Size is 0."));
  }
  myCAM.CS_LOW();
  myCAM.set_fifo_burst();

  i = 0;
  while (len--) {
    temp_last = temp;
    temp = SPI.transfer(0x00);
    
    //Read JPEG data from FIFO
    if ((temp == 0xD9) && (temp_last == 0xFF))  //If find the end ,break while,
    {
      buffer[i++] = temp;  //save the last  0XD9
      
      postServer(buffer, i);

      is_header = false;
      i = 0;
      myCAM.CS_HIGH();
      break;
    }
    if (is_header == true) {
      //Write image data to buffer if not full
      if (i < bufferSize) { 
        buffer[i++] = temp;
      }
      else {
        //Write bufferSize bytes image data to file
        Serial.println("DOING THIS RIGHT HERE OMG ALSFKJAL;SDJKFA;SDLKJFAS;DLKFJASDL;FKJADS;LKFJASD;LKFJASL;DFKJASD");
        i = 0;
        buffer[i++] = temp;
      }
    } else if ((temp == 0xD8) & (temp_last == 0xFF)) {
      is_header = true;
      buffer[i++] = temp_last;
      buffer[i++] = temp;
    }
  }
}

 
void postServer(uint8_t * data, int index) {
  request2[0] = '\0';
  sprintf(request2,"POST /sandbox/sc/team40/imageLoad.py HTTP/1.1\r\n");
  strcat(request2,"Host: 608dev-2.net\r\n");
  strcat(request2,"Content-Type: application/x-www-form-urlencoded\r\n");
  sprintf(request2+strlen(request2),"Content-Length: %ld\r\n", 2*index+18);
  strcat(request2,"\r\n"); 
  strcat(request2, "data="); //body

  for(int k=0; k<index; k++){
    sprintf(request2+strlen(request2), "%02x", data[k]);
  }
  strcat(request2, "&user=arduino");

  strcat(request2,"\r\n\r\n"); //new line
  do_http_request("608dev-2.net", request2, newmode, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT,true);
  // Serial.println(newmode);
}

void getServer() {
 
  sprintf(request,"GET http://608dev-2.net/sandbox/sc/team40/imageLoad.py HTTP/1.1\r\n");
  strcat(request,"Host: 608dev-2.net\r\n");
  strcat(request,"\r\n");
  do_http_request("608dev-2.net", request, getresponse, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT,true);
  // Serial.println("Response from get server: ");
  // Serial.println(getresponse);
}

void getLocationCoords(char* location) {
  request[0] = '\0'; //set 0th byte to null
  sprintf(request, "GET /sandbox/sc/team40/geocoding.py?location=%s HTTP/1.1\r\n", location);
  strcat(request,"Host: 608dev-2.net\r\n");
  strcat(request,"\r\n");
  Serial.println(request);
  do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
}

int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}
 

void setup() {
  //client.setCACert(CA_CERT); //set cert for https
  Serial.begin(115200);

  tft.init();
  tft.setRotation(2);
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);


  pinMode(BUTTON1, INPUT_PULLUP);
  oldstate = -1;
  WiFi.mode(WIFI_STA);
    uint8_t vid, pid;
  uint8_t temp;
  //set the CS as an output:
  pinMode(CS, OUTPUT);
  pinMode(CAM_POWER_ON, OUTPUT);
  digitalWrite(CAM_POWER_ON, HIGH);
#if defined(__SAM3X8E__)
  Wire1.begin();
#else
  Wire.begin();
#endif
  Serial.begin(115200);
  Serial.println(F("ArduCAM Start!"));

  // initialize SPI:
  SPI.begin();
  SPI.setFrequency(4000000);  //4MHz

  //BRING BACK

  //Check if the ArduCAM SPI bus is OK
  myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
  temp = myCAM.read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55) {
    Serial.println(F("SPI1 interface Error!"));
    while (1)
      ;
  }

  //Check if the ArduCAM SPI bus is OK
  myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
  temp = myCAM.read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55) {
    Serial.println(F("SPI1 interface Error!"));
    while (1)
      ;
  }
#if defined(OV2640_MINI_2MP) || defined(OV2640_CAM)
  //Check if the camera module type is OV2640
  myCAM.wrSensorReg8_8(0xff, 0x01);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
  if ((vid != 0x26) && ((pid != 0x41) || (pid != 0x42)))
    Serial.println(F("Can't find OV2640 module!"));
  else
    Serial.println(F("OV2640 detected."));
#elif defined(OV5640_MINI_5MP_PLUS) || defined(OV5640_CAM)
  //Check if the camera module type is OV5640
  myCAM.wrSensorReg16_8(0xff, 0x01);
  myCAM.rdSensorReg16_8(OV5640_CHIPID_HIGH, &vid);
  myCAM.rdSensorReg16_8(OV5640_CHIPID_LOW, &pid);
  if ((vid != 0x56) || (pid != 0x40))
    Serial.println(F("Can't find OV5640 module!"));
  else
    Serial.println(F("OV5640 detected."));
#elif defined(OV5642_MINI_5MP_PLUS) || defined(OV5642_MINI_5MP) || defined(OV5642_MINI_5MP_BIT_ROTATION_FIXED) || (defined(OV5642_CAM))
  //Check if the camera module type is OV5642
  myCAM.wrSensorReg16_8(0xff, 0x01);
  myCAM.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
  myCAM.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
  if ((vid != 0x56) || (pid != 0x42)) {
    Serial.println(F("Can't find OV5642 module!"));
  } else
    Serial.println(F("OV5642 detected."));
#endif


  //Change to JPEG capture mode and initialize the OV2640 module
  myCAM.set_format(JPEG);
  myCAM.InitCAM();
#if defined(OV2640_MINI_2MP) || defined(OV2640_CAM)
  myCAM.OV2640_set_JPEG_size(OV2640_160x120);
#elif defined(OV5640_MINI_5MP_PLUS) || defined(OV5640_CAM)
  myCAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);  //VSYNC is active HIGH
  myCAM.OV5640_set_JPEG_size(OV5640_320x240);
#elif defined(OV5642_MINI_5MP_PLUS) || defined(OV5642_MINI_5MP) || defined(OV5642_MINI_5MP_BIT_ROTATION_FIXED) || (defined(OV5642_CAM))
  myCAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);  //VSYNC is active HIGH
  myCAM.OV5640_set_JPEG_size(OV5642_320x240);
#endif

  myCAM.clear_fifo_flag();

    //if using regular connection use line below:
    WiFi.begin(network, passdecodedWord);
    uint8_t count = 0; //count used for Wifi check times
    Serial.print("Attempting to connect to ");
    Serial.println(network);
    while (WiFi.status() != WL_CONNECTED && count<6) {
      delay(500);
      Serial.print(".");
      count++;
    }
    delay(2000);
    if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
      Serial.println("CONNECTED!");
      Serial.printf("%d:%d:%d:%d (%s) (%s)\n",WiFi.localIP()[3],WiFi.localIP()[2],
                                              WiFi.localIP()[1],WiFi.localIP()[0], 
                                            WiFi.macAddress().c_str() ,WiFi.SSID().c_str());
      delay(500);
    } else { //if we failed to connect just Try again.
      Serial.println("Failed to Connect :/  Going to restart");
      Serial.println(WiFi.status());
      ESP.restart(); // restart the ESP (proper way)
    }
  setUpMP3(); //BRING BACK

  returnval = 0;
  
}
 
void loop() {
  /* uncomment this for actually running it */
  start_capture();
  Serial.println("before while loop");
  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
  Serial.println("after while loop");
  readImage();
  Serial.println(state);
  //Serial.println("before get server");
  getServer();
  //Serial.println("after get server");
  returnval = atoi(getresponse); //atoi would give 0 on empty
  getresponse[0] = '\0';
  gesture(state, returnval);
  Serial.println("after main state machine call");

}
 
void gesture(uint8_t curstate, uint8_t stat) {
  Serial.print("in gesture: ");
  Serial.println(curstate);
  switch(curstate) {
    case BASE:
    {
      Serial.println("Current State: BASE");

      if ((stat != -1) && (stat == 1 || stat == 2)) {
        if (stat ==1) {
          Serial.println("Switching to Media");
          playCallout(MEDIA_CALLOUT);
        }
        if (stat ==2) {
          Serial.println("Switching to Navigation");
         playCallout(NAVIGATION_CALLOUT);
        }
        state = stat;
      }
      break;
    }

    case MEDIA:
    {
      Serial.println("Current State: MEDIA");
      Serial.print("Wifi State: ");
      Serial.println(WiFi.SSID());

      if (stat == RESET) {
        Serial.println("Switching to Base Mode");
        playCallout(SELECT_MODE_CALLOUT);
        state = BASE;
      }

      else if (stat==PLAY /*&& !playingMedia*/) {
        Serial.println("PLAYING");
        playCallout(curr_song);

        state = MEDIA;
        playingMedia = true;
        pausingMedia = false;
      }

      else if (stat==PAUSE /*&& !pausingMedia*/) {
        //send 3 via ESP NOW
        Serial.println("PAUSING");

        myDFPlayer.pause();

        state = MEDIA;
        pausingMedia = true;
        playingMedia = false;
      }

      else if (stat==NEXT && !nextingMedia) {
        Serial.println("Playing New Song");
        if (MIN_SONG <= curr_song + 1 && curr_song + 1 <= MAX_SONG) {
          curr_song += 1;
        }
        playCallout(curr_song);

        state = MEDIA;
        playingMedia = true;
        pausingMedia = false;
      }

      else if (stat==PREV) {
        Serial.println("Playing previous Song");
        if (MIN_SONG <= curr_song - 1 && curr_song - 1 <= MAX_SONG) {
          curr_song -= 1;
        }
        playCallout(curr_song);

        state = MEDIA;
        playingMedia = true;
        pausingMedia = false;
      }

      break;
    }
    case NAVIGATION:
      

      {
        Serial.println("starting nav");
        if (stat == RESET) {
        Serial.println("Switching to Base Mode");
       playCallout(SELECT_MODE_CALLOUT);
        state = BASE;
      }
      int offset = sprintf(json_body, "%s", PREFIX);
      int n = WiFi.scanNetworks(); //run a new scan. could also modify to use original scan from setup so quicker (though older info)
      //Serial.println("scan done");
      if (n == 0) {
        Serial.println("no networks found");
      } else {
        Serial.println("1");
        int max_aps = max(min(MAX_APS, n), 1);
        for (int i = 0; i < max_aps; ++i) { //for each valid access point
          uint8_t* mac = WiFi.BSSID(i); //get the MAC Address
          offset += wifi_object_builder(json_body + offset, JSON_BODY_SIZE-offset, WiFi.channel(i), WiFi.RSSI(i), WiFi.BSSID(i)); //generate the query
          if(i!=max_aps-1){
            offset +=sprintf(json_body+offset,",");//add comma between entries except trailing.
          }
        }
        sprintf(json_body + offset, "%s", SUFFIX);
        //Serial.println(json_body);
        int len = strlen(json_body);

        request[0] = '\0'; //set 0th byte to null
        offset = 0; //reset offset variable for sprintf-ing
        offset += sprintf(request + offset, "POST https://www.googleapis.com/geolocation/v1/geolocate?key=%s  HTTP/1.1\r\n", API_KEY);
        offset += sprintf(request + offset, "Host: googleapis.com\r\n");
        offset += sprintf(request + offset, "Content-Type: application/json\r\n");
        offset += sprintf(request + offset, "cache-control: no-cache\r\n");
        offset += sprintf(request + offset, "Content-Length: %d\r\n\r\n", len);
        offset += sprintf(request + offset, "%s\r\n", json_body);
        do_https_request(SERVER, request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);

        Serial.println("here");

        DynamicJsonDocument doc(300);
        char* firstPosition = strchr(response, '{');
        char* lastPosition = strrchr(response, '}');  
        char json[200];
        
        //splice string
        strncpy(json, firstPosition, lastPosition - firstPosition + 1);
        json[lastPosition - firstPosition+1] = '\0';          
              
        DeserializationError error = deserializeJson(doc, json);
        // Test if parsing succeeds.
        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
          return;
        }   

        double lat = doc["location"]["lat"];
        double lng = doc["location"]["lng"];

        Serial.println(lat);
        Serial.println(lng);

        //TODO CHANGE TO TAKE IN USER VOICE INPUT HERE
        getAudio();
        getLocationCoords(finalword);   
      
        Serial.println("second req");

        StaticJsonDocument<100> doc2;
        DeserializationError error2 = deserializeJson(doc2, response);

        // Test if parsing succeeds.
        if (error2) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
          return;
        }

        double dest_long = doc2["longitude"];
        double dest_lat = doc2["latitude"];

        Serial.println(dest_long);
        Serial.println(dest_lat);

        if (millis() - timerdot > 5000) {
          request[0] = '\0';
          body[0] = '\0';
          sprintf(body,"startlat=%f&startlon=%f&destlat=%f&destlon=%f", lat, lng, dest_lat, dest_long);//generate body, posting to User, 1 step
          int body_len = strlen(body); //calculate body length (for header reporting)
          sprintf(request,"POST http://608dev-2.net/sandbox/sc/team40/navigate.py HTTP/1.1\r\n");
          strcat(request,"Host: 608dev-2.net\r\n");
          strcat(request,"Content-Type: application/x-www-form-urlencoded\r\n");
          sprintf(request+strlen(request),"Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
          strcat(request,"\r\n"); //new line from header to body
          strcat(request,body); //body
          strcat(request,"\r\n"); //new line
          
          //Serial.println(request_buffer);
          do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT,true);
        // Serial.println("Directions to Prudential Center: ");
          Serial.println(response); //viewable in Serial Terminal
          tft.println(response);
          timerdot = millis();
        }
      }
    }
  oldstate = curstate;
  }
}

void getAudio() {
  button_state = digitalRead(BUTTON1);

  // record_audio();
  if (!button_state && button_state != old_button_state) {
    tft.fillScreen(TFT_BLACK); //fill background
    tft.setCursor(0,0, 1);
    finalword[0] = '\0';
    Serial.println("listening...");

     Serial.println("started record");

    int sample_num = 0;    // counter for samples
    int enc_index = strlen(audioPREFIX) - 1;  // index counter for encoded samples
    float time_between_samples = 1000000 / SAMPLE_FREQ;
    int value = 0;
    char raw_samples[3];   // 8-bit raw sample data array
    memset(speech_data, 0, sizeof(speech_data));
    sprintf(speech_data, "%s", audioPREFIX);
    char holder[5] = {0};
    Serial.println("starting");
    uint32_t text_index = enc_index;
    uint32_t start = millis();
    time_since_sample = micros();
    
    Serial.println("actually recording");

    while (sample_num < NUM_SAMPLES && !digitalRead(BUTTON1)) { //read in NUM_SAMPLES worth of audio data
      value = analogRead(AUDIO_IN);  //make measurement
      raw_samples[sample_num % 3] = mulaw_encode(value - 1800); //remove 1.5ishV offset (from 12 bit reading)
      sample_num++;
      if (sample_num % 3 == 0) {
        base64_encode(holder, raw_samples, 3);
        strncat(speech_data + text_index, holder, 4);
        text_index += 4;
      }
      // wait till next time to read
      while (micros() - time_since_sample <= time_between_samples); //wait...
      time_since_sample = micros();
    }
    Serial.println(millis() - start);
    sprintf(speech_data + strlen(speech_data), "%s", audioSUFFIX);
    Serial.println("out");
    
    Serial.println("sending...");
    Serial.print("\nStarting connection to server...");
    delay(300);
    bool conn = false;
    for (int i = 0; i < 10; i++) {
      int val = (int)client.connect(audioSERVER, 443, 4000);
      Serial.print(i); Serial.print(": "); Serial.println(val);
      if (val != 0) {
        conn = true;
        break;
      }
      Serial.print(".");
      delay(300);
    }
    if (!conn) {
      Serial.println("Connection failed!");
      return;
    } else {
      Serial.println("Connected to server!");
      Serial.println(client.connected());
      int len = strlen(speech_data);
      // Make a HTTP request:
      client.print("POST /v1/speech:recognize?key="); client.print(API_KEY); client.print(" HTTP/1.1\r\n");
      client.print("Host: speech.googleapis.com\r\n");
      client.print("Content-Type: application/json\r\n");
      client.print("cache-control: no-cache\r\n");
      client.print("Content-Length: "); client.print(len);
      client.print("\r\n\r\n");
      int ind = 0;
      int jump_size = 1000;
      char temp_holder[jump_size + 10] = {0};
      Serial.println("sending data");
      while (ind < len) {
        delay(80);//experiment with this number!
        //if (ind + jump_size < len) client.print(speech_data.substring(ind, ind + jump_size));
        strncat(temp_holder, speech_data + ind, jump_size);
        client.print(temp_holder);
        ind += jump_size;
        memset(temp_holder, 0, sizeof(temp_holder));
      }
      client.print("\r\n");
      Serial.print("\r\n\r\n");
      Serial.println("Through send...");
      unsigned long count = millis();
      while (client.connected()) {
        Serial.println("IN!");
        String line = client.readStringUntil('\n');
        Serial.print(line);
        if (line == "\r") { //got header of response
          Serial.println("headers received");
          break;
        }
        if (millis() - count > RESPONSE_TIMEOUT) break;
      }
      Serial.println("");
      Serial.println("Response...");
      count = millis();
      while (!client.available()) {
        delay(100);
        Serial.print(".");
        if (millis() - count > RESPONSE_TIMEOUT) break;
      }
      Serial.println();
      Serial.println("-----------");
      memset(response, 0, sizeof(response));
      while (client.available()) {
        char_append(response, client.read(), OUT_BUFFER_SIZE);
      }
      Serial.println(response); //comment this out if needed for debugging
      char* trans_id = strstr(response, "transcript");
      if (trans_id != NULL) {
        char* foll_coll = strstr(trans_id, ":");
        char* starto = foll_coll + 3; //starting index
        char* endo = strstr(starto + 1, "\""); //ending index
        int transcript_len = endo - starto;
        char transcript[100] = {0};
        strncat(transcript, starto, transcript_len);
        strcpy(finalword, transcript);
        Serial.println(transcript);

      }
      Serial.println("-----------");
      client.stop();
      Serial.println("done");
    }
  }
  old_button_state = 1;
}
 
void setUpMP3() {
  mySoftwareSerial.begin(9600, SERIAL_8N1, 32, 33);  // speed, type, RX, TX
  delay(5000);
  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  delay(5000);
  while (!myDFPlayer.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
   
    Serial.println(myDFPlayer.readType(),HEX);
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
  }
  Serial.println(F("DFPlayer Mini online."));
 
  myDFPlayer.setTimeOut(500); //Set serial communictaion time out 500ms
 
  //----Set volume----
  myDFPlayer.volume(20);  //Set volume value (0~30).
  myDFPlayer.volumeUp(); //Volume Up
  myDFPlayer.volumeDown(); //Volume Down
 
  //----Set different EQ----
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
 
  //----Set device we use SD as default----
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
 
  int delayms=100;
 
}
 
void playCallout(int callout) {
  myDFPlayer.playMp3Folder(callout);  //Play the first mp3
}

//function used to record audio at sample rate for a fixed nmber of samples
void record_audio() {
  Serial.println("started record");
  int sample_num = 0;    // counter for samples
  int enc_index = strlen(audioPREFIX) - 1;  // index counter for encoded samples
  float time_between_samples = 1000000 / SAMPLE_FREQ;
  int value = 0;
  char raw_samples[3];   // 8-bit raw sample data array
  memset(speech_data, 0, sizeof(speech_data));
  Serial.println("record here");
  sprintf(speech_data, "%s", audioPREFIX);
  char holder[5] = {0};
  Serial.println("starting");
  uint32_t text_index = enc_index;
  uint32_t start = millis();
  time_since_sample = micros();
  while (sample_num < NUM_SAMPLES && !digitalRead(BUTTON1)) { //read in NUM_SAMPLES worth of audio data
    value = analogRead(AUDIO_IN);  //make measurement
    raw_samples[sample_num % 3] = mulaw_encode(value - 1800); //remove 1.5ishV offset (from 12 bit reading)
    sample_num++;
    if (sample_num % 3 == 0) {
      base64_encode(holder, raw_samples, 3);
      strncat(speech_data + text_index, holder, 4);
      text_index += 4;
    }
    // wait till next time to read
    while (micros() - time_since_sample <= time_between_samples); //wait...
    time_since_sample = micros();
  }
  Serial.println(millis() - start);
  sprintf(speech_data + strlen(speech_data), "%s", audioSUFFIX);
  Serial.println("out");
}
 
