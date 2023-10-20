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
uint8_t rightHandMac[] = {0x40, 0xF5, 0x20, 0x58, 0x34, 0x08}; //set right hand ESP32 mac address

// MPU6050 setup
Adafruit_MPU6050 mpu;

// 8x8 LED setup
const int DIN = 27; //setup DIN pin
const int CS = 26; //setup CS pin
const int CLK = 25; //setup CLK pin
LedControl lc = LedControl(DIN, CLK, CS, 0);

//data receive setup
int touchOkValue; //touch ok value from right hand ESP32
int punchRCount; //right punch count from right hand ESP32

//left punch count
int punchLCount = 0; //set the left punch count to 0

//Touch Threshold
const int threshold = 20;

//define game state
typedef enum
{
  ARCADE,
  TIME,
  VS
} stateMachine;

stateMachine currentState = ARCADE; //select arcade mode after initializing, but will change depending on right hand state

bool logoDisplayed = false;
bool espNowConnected = false; //set to false to prevent overlap with the right hand ESP32. Can be activated after receive data from right hand ESP32

byte *currentLightning; //variable for current lightning in 8x8 LED
byte *currentHeart; //variable for current heart in 8x8 LED

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
  if (len == sizeof(uint8_t) * 3)
  {
    currentState = static_cast<stateMachine>(data[0]);
    touchOkValue = static_cast<int>(data[1]);
    punchRCount = static_cast<int>(data[2]);
    Serial.print("Received state: ");
    Serial.println(currentState);
    Serial.print("Received touchOkValue: ");
    Serial.println(touchOkValue);
    Serial.print("Received punchRCount: ");
    Serial.println(punchRCount);

    espNowConnected = true; //change the state of the ESP-NOW connection after succesfully received data
  }
  else
  {
    Serial.println("Invalid data length received");
    espNowConnected = false;
  }
}

void setup()
{
  Serial.begin(115200);

  //Initialize Wi-Fi and ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  esp_now_register_send_cb(OnDataSent); //set callback on data sent
  esp_now_register_recv_cb(OnDataRecv); //set callback on data receive

  //Add the right-hand ESP32 to the peer list
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, rightHandMac, sizeof(rightHandMac));
  peerInfo.channel = 0; //Set the channel
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
  }

  while (!espNowConnected) //wait for the two ESP32 for connected
  {
    delay(1000);
    Serial.println("Waiting for ESP-NOW connection...");
  }

  //Set up the buzzer
  pinMode(BUZZER_PIN, OUTPUT);

  //LED setup
  lc.shutdown(0, false);
  lc.setIntensity(0, 10); //LED light intensity
  lc.clearDisplay(0);

  //MPU6050 setup
  //Initialize MPU6050
  if (!mpu.begin())
  {
    Serial.println("Failed to initialize MPU6050 sensor. Check connections.");
    while (1)
      ;
  }

  //Configure MPU6050 settings
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G); //Set the accelerometer range to +/-2g

  //Starting up
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
  switch (currentState) //start with current state (arcade mode) 
  {
    case ARCADE: 
      DisplayBytes (lightning16); //display arcade logo
      if (touchOkValue < threshold) //if ok button pin touched
      {
        playOk(); //ok sound
        startArcade(); //start arcade mode
      }
      break;
    case TIME:
      DisplayBytes (timeup); //display time attack logo
      if (touchOkValue < threshold) //if ok button pin touched
      {
        playOk(); //ok sound
        startTime(); //start time attack mode
      }
      break;
    case VS:
      DisplayBytes (versus); //display versus logo
      if (touchOkValue < threshold) //if ok button pin touched
      {
        playOk(); //ok sound
        startVersus(); //start versus mode
      }
      break;
    }
  delay(10);
}

void sendPunchLCount(int punchLCount)
{
  uint8_t data[1]; //Create a byte array to hold the state information
  data[0] = static_cast<uint8_t>(punchLCount);

  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, rightHandMac, sizeof(rightHandMac));
  peerInfo.channel = 0; //Channel on which the left hand ESP32 is listening

  // Send the state to the left hand ESP32
  if (esp_now_send(peerInfo.peer_addr, data, sizeof(data)) != ESP_OK)
  {
    Serial.println("Failed to send state");
  }
}

void DisplayBytes(byte image[]) //displaying image from hex byte 
{
  for (int i = 0; i <= 7; i++)
  {
    lc.setRow(0, i, image[i]);
  }
}

void getReady() //countdown display and sound for game mode transition
{
  delay(300);
  DisplayBytes(three);
  tone(BUZZER_PIN, NOTE_B5, 500);
  delay(1000);
  noTone(BUZZER_PIN);
  DisplayBytes(two);
  tone(BUZZER_PIN, NOTE_B5, 500);
  delay(1000);
  noTone(BUZZER_PIN);
  DisplayBytes(one);
  tone(BUZZER_PIN, NOTE_B5, 500);
  delay(1000);
  noTone(BUZZER_PIN);
  DisplayBytes(go);
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

  while (punchLCount < targetCount) //activate left punch function until right hand reached the target
  {
    punchL();
    
    if (punchLCount >= i) 
    {
      currentLightning = lightningArrays[i];
      DisplayBytes(currentLightning);
      i++; //Increment index when the punchRCount matches
    }
  }

  while (punchLCount == targetCount && punchRCount < targetCount) //wait for right hand finished reach the punching target
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

  delay(500);
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

  while (millis() - startTime < duration) //activate left punch function after the timer start and create timer animation
  {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) //create pattern interval within timer duration
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

    punchL();
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
  int punches = punchLCount + punchRCount;
  if (punches >= 10 && punches <= 320)
  {
    int index = punches / 20; //Calculate the index in the timearrays array
    DisplayBytes(timeArrays[index]); //Display the corresponding total punch pattern
    delay(5500);
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

  int i = 0; //Initialize i to 0

  while (punchLCount < 60)
  {
    punchL();

    if (punchRCount >= i * 10)
    {
      currentHeart = heartArrays[6 - i]; //Decrease the heart from full to zero
      DisplayBytes(currentHeart);
      i++; //Increment index when the punchLCount matches

      if (punchRCount == 60)
      {
        break;
      }
    }
  }

  delay(100);

  if (punchRCount < punchLCount && punchLCount == 60) //Player 2 Win, play victory song and icon
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

    punchRCount = 0; //reset right punch count
    punchLCount = 0; //reset left punch count
  }

  else if (punchLCount < punchRCount && punchRCount == 60) //Player 2 lose, play lose song and icon
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

  else if (punchLCount + punchRCount == 120) //Tie, play lose song and icon
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
  delay(1500); 
}

void punchL() //left hand punch count
{
  sensors_event_t a, g, temp;

  //Read MPU6050 sensor data
  mpu.getEvent(&a, &g, &temp);

  //Check for a punchL (acceleration along the y-axis)
  if (a.acceleration.y <= -9 || a.acceleration.y >= 9)
  {
    punchLCount++; //Increment punchLCount if a punchL is detected
    playCoinSound();
    sendPunchLCount(punchLCount);

    delay(200);
  }

  Serial.print("punchLCount: ");
  Serial.println(punchLCount);
}