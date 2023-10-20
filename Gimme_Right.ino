#include <esp_now.h>
#include <WiFi.h>
#include <LedControl.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>       
#include <pgmspace.h>
#include <pitches.h>
#include "LEDFaces.h"
#include "sound.h"
#define BUZZER_PIN 19


//ESPNOW setup
uint8_t leftHandMac[] = {0x94, 0xE6, 0x86, 0xA7, 0xC1, 0x24}; //set left hand ESP32 mac address

//MPU6050 setup
Adafruit_MPU6050 mpu;

//touchpin setup
const int nextPin = 4; //set touch pin for next button
const int okPin = 12; //set touch pin for ok button
const int threshold = 20; //threshold for touch pin
int touchNextValue; //variable for next button touch pin
int touchOkValue; //variable for ok button pin

//8x8 LED setup
const int DIN = 27; //setup DIN pin
const int CS = 26; //setup CS pin
const int CLK = 25; //setup CLK pin
LedControl lc = LedControl(DIN,CLK,CS,0);

//data receive setup
int punchLCount; //variable for left hand punch count
int punchRCount = 0; //set left hand punch count to 0

//define game state
typedef enum  
{
  ARCADE,
  TIME,
  VS
} stateMachine;

stateMachine currentState = ARCADE; //select arcade mode after initializing

bool logoDisplayed = false;

byte* currentLightning; //variable for current lightning in 8x8 LED
byte* currentHeart; //variable for current heart in 8x8 LED

void OnDataSent(const uint8_t *mac, esp_now_send_status_t status) //show status while sending data
{
  if (status == ESP_NOW_SEND_SUCCESS)
  {
    Serial.println("Data sent successfully");
  }
  else 
  {
    Serial.println("Data failed to send");
  }
}

void OnDataRecv(const uint8_t *mac, const uint8_t *data, int len) //show status while receive data
{
  if (len == sizeof(uint8_t) * 1)
  {
    punchLCount = static_cast<int>(data[0]);
    Serial.print("Received punchLCount: ");
    Serial.println(punchLCount);
  }
  else
  {
    Serial.println("Invalid data length received");
  }
}

void setup()
{
  Serial.begin(115200);

  //Initialize WiFi and ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  esp_now_register_send_cb(OnDataSent); //set callback on data sent
  esp_now_register_recv_cb(OnDataRecv); //set callback on data receive

  //Add the left hand ESP32 to the peer list
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, leftHandMac, sizeof(leftHandMac));
  peerInfo.channel = 0; //Set the channel
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
  }

  //Set up the touch pins as inputs
  pinMode(nextPin, INPUT);
  pinMode(okPin, INPUT);

  //Set up the buzzer
  pinMode (BUZZER_PIN, OUTPUT);

  //LED setup
  lc.shutdown (0, false);
  lc.setIntensity (0, 10); //LED light intensity
  lc.clearDisplay (0);

  //MPU6050 setup
  //Initialize MPU6050
  if (!mpu.begin())
  {
    Serial.println("Failed to initialize MPU6050 sensor. Check connections.");
    while (1);
  }

  //Configure MPU6050 settings
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G); //Set the accelerometer range to +/-2g

  //Starting up
  sendState(currentState, touchOkValue, punchRCount); //send state to left hand ESP32
  DisplayBytes(connect); //display connected logo
  playOk(); //Ok sound
  delay(2000);
  DisplayBytes(logo); //display gimme! logo
  playLose(); //lose sound
  delay(2500);
  logoDisplayed = true;
  lc.clearDisplay(0);
  delay(1000);
}

void loop()
{
  int touchNextValue = touchRead(nextPin);
  int touchOkValue = touchRead(okPin);

  if (!logoDisplayed) { //check logo displayed status
    DisplayBytes(logo);
    delay(5000); 
    logoDisplayed = true;
    lc.clearDisplay(0);
    delay(1000);
  } 
  else 
  {
    if (touchNextValue < threshold) 
    {
      currentState = static_cast<stateMachine>((currentState + 1) % 3); //if next button pin touched
      playNext(); //move to next state
      sendState(currentState, touchOkValue, punchRCount); //send the state to left hand ESP32
    }

    switch (currentState) //start with current state (arcade mode) 
    {
      case ARCADE:
        DisplayBytes (lightning16); //display arcade logo
        sendState(currentState, touchOkValue, punchRCount); //send the state to left hand ESP32
        if (touchOkValue < threshold) //if ok button pin touched
        {
          playOk(); //ok sound
          startArcade(); //start arcade mode
        }
        break;
      case TIME:
        DisplayBytes (timeup); //display time attack logo
        sendState(currentState, touchOkValue, punchRCount); //send the state to left hand ESP32
        if (touchOkValue < threshold) //if ok button pin touched
        {
          playOk(); //ok sound
          startTime(); //start time attack mode
        }
        break;
      case VS:
        DisplayBytes (versus); //display versus logo
        sendState(currentState, touchOkValue, punchRCount); //send the state to left hand ESP32
        if (touchOkValue < threshold) //if ok button pin touched
        {
          playOk(); //ok sound
          startVersus(); //start versus mode
        }
        break;
    }
    delay(10);
  }
}

void sendState(stateMachine state, int touchOkValue, int punchRCount) //send state, touch ok value, and right punch count to left hand ESP32
{ 
  uint8_t data[3]; //Create a byte array to hold the state information
  data[0] = static_cast<uint8_t>(state);
  data[1] = static_cast<uint8_t>(touchOkValue);
  data[2] = static_cast<uint8_t>(punchRCount);

  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, leftHandMac, sizeof(leftHandMac));
  peerInfo.channel = 0; //Channel on which the left hand ESP32 is listening

  if (esp_now_send(peerInfo.peer_addr, data, sizeof(data)) != ESP_OK)
  {
    Serial.println("Failed to send state");
  }
}

void DisplayBytes(byte image[]) //displaying image from hex byte 
{
  for(int i =0; i<=7; i++)
  {
    lc.setRow(0,i, image[i]);
  }
}

void getReady() //countdown display and sound for game mode transition
{
  delay(300);
  DisplayBytes (three);
  tone(BUZZER_PIN, NOTE_B5, 500);
  delay(1000);
  noTone(BUZZER_PIN);
  DisplayBytes (two);
  tone(BUZZER_PIN, NOTE_B5, 500);
  delay(1000);
  noTone(BUZZER_PIN);
  DisplayBytes (one);
  tone(BUZZER_PIN, NOTE_B5, 500);
  delay(1000);
  noTone(BUZZER_PIN);
  DisplayBytes (go);
  tone(BUZZER_PIN, NOTE_E6, 800);
  delay(1000);
  noTone(BUZZER_PIN);
}

void startArcade() //start arcade mode
{
  int targetCount = 16; //punch target
  getReady();
  delay(500);
  lc.clearDisplay(0);

  int i = 0; //Initialize index to zero

  while (punchRCount < targetCount) //activate right punch function until right hand reached the target
  {
    punchR();
    
    if (punchRCount >= i) 
    {
      currentLightning = lightningArrays[i];
      DisplayBytes(currentLightning);
      i++; //Increment index when the punchRCount matches
    }
  }

  while (punchRCount == targetCount && punchLCount < targetCount) //wait for left hand finished reach the punching target
  {
    DisplayBytes(lightning16);
    playOk();
  }

  if (punchRCount == targetCount && punchLCount == targetCount) //after right hand and left hand reach punching target
  {
    lc.clearDisplay(0);
    playOk();
    delay(100);
    DisplayBytes(lightning16);
    playOk();
    delay(100);
    lc.clearDisplay(0);
    playOk();
    delay(100);
    DisplayBytes(lightning16);
    playOk();
    delay(100);
    lc.clearDisplay(0);
    playOk();
    delay(100);
    DisplayBytes(win);
    playVictory();
    delay(3000);
  }

  punchRCount = 0; //reset right punch count
  punchLCount = 0; //reset left punch count
}

void startTime() //start time attack mode
{
  getReady();
  delay(500);

  unsigned long startTime = millis(); 
  unsigned long duration = 60000; //set time to one minute
  unsigned long previousMillis = 0;
  const long interval = 500; //interval between time pattern
  int patternIndex = 0; 

  while (millis() - startTime < duration) //activate right punch function after the timer start and create timer animation
  { 
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval)  //create pattern interval within timer duration
    {
      previousMillis = currentMillis;

      if (patternIndex == 0)
      {
        DisplayBytes(timeup);
        patternIndex = 1;
      } 
      else 
      {
        DisplayBytes(timedown);
        patternIndex = 0;
      }
    }

    punchR();
  }

  //Play victory song after the timer ended
  lc.clearDisplay(0);
  playOk();
  delay(100);
  DisplayBytes(timeup);
  playOk();
  delay(100);
  lc.clearDisplay(0);
  playOk();
  delay(100);
  DisplayBytes(timeup);
  playOk();
  delay(100);
  lc.clearDisplay(0);
  playOk();
  delay(100);
  DisplayBytes(win);
  playVictory();
  delay(800);
  
  //Display the final score
  int punches = punchRCount + punchLCount;
  if (punches >= 10 && punches <= 320) {
    int index = punches / 20; //Calculate the index in the timearrays array
    DisplayBytes(timeArrays[index]); //Display the corresponding total punch pattern
    delay(5000);
  }
  
  punchRCount = 0; //Reset right punch count
  punchLCount = 0; //Reset left punch count
}

void startVersus() //start versus mode
{
  getReady();
  delay(500);
  currentHeart = heart06; //Set currentHeart to full heart
  DisplayBytes(currentHeart); //Display full heart
  lc.clearDisplay(1);

  int i = 0; //Initialize index to 0

  while (punchRCount < 60)
  {
    punchR();

    if (punchLCount >= i * 10) 
    {
      currentHeart = heartArrays[6 - i]; //Decrease the heart from full to zero
      DisplayBytes(currentHeart);
      i++; //Increment index when the punchRCount matches
      
      if (punchLCount == 60)
      {
        break;
      }
    }
  }

  delay(100);

  if (punchLCount < punchRCount && punchRCount == 60) //Player 1 Win, play victory song and icon
  {
    DisplayBytes(currentHeart);
    delay(500);
    lc.clearDisplay(0);
    delay(500);
    DisplayBytes(currentHeart);
    delay(500);
    lc.clearDisplay(0);
    delay(500);
    DisplayBytes(currentHeart);
    delay(500);
    lc.clearDisplay(0);
    delay(500);
    DisplayBytes(win);
    playVictory();
    delay(500);

    punchRCount = 0;
    punchLCount = 0;
  }

  else if (punchRCount < punchLCount && punchLCount == 60) //Player 1 lose, play lose song and icon
  {
    DisplayBytes(heart00);
    delay(500);
    lc.clearDisplay(0);
    delay(500);
    DisplayBytes(heart00);
    delay(500);
    lc.clearDisplay(0);
    delay(500);
    DisplayBytes(heart00);
    delay(500);
    lc.clearDisplay(0);
    delay(500);
    DisplayBytes(lose);
    playLose();
    delay(500);

    punchRCount = 0;
    punchLCount = 0;
  }

  else if (punchRCount + punchLCount == 120) //Tie, play lose song and icon
  {
    DisplayBytes(heart00);
    delay(500);
    lc.clearDisplay(0);
    delay(500);
    DisplayBytes(heart00);
    delay(500);
    lc.clearDisplay(0);
    delay(500);
    DisplayBytes(heart00);
    delay(500);
    lc.clearDisplay(0);
    delay(500);
    DisplayBytes(lose);
    playLose();
    delay(500);

    punchRCount = 0; //reset right punch count
    punchLCount = 0; //reset left punch count
  }
  delay(200);
}

void punchR() //right punch function
{
  sensors_event_t a, g, temp;

  //Read MPU6050 sensor data
  mpu.getEvent(&a, &g, &temp);

  //Check for a punchR (acceleration along the y-axis)
  if (a.acceleration.y <= -9 || a.acceleration.y >= 9)
  {
    punchRCount++; //Increment punchRCount if a punchR is detected
    playCoinSound();
    sendState(currentState, touchOkValue, punchRCount); //send right punch count to left hand ESP32

    delay(200);
  }

  Serial.print("punchRCount: ");
  Serial.println(punchRCount);
}