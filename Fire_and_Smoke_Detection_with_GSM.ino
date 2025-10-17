#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

// I2C LCD Configuration
LiquidCrystal_I2C lcd(0x27, 16, 2);

// GSM Module Configuration
SoftwareSerial gsmSerial(2, 3); // RX, TX (Connect GSM TX->D2, GSM RX->D3)
const String PHONE_NUMBER = "+1234567890"; // Replace with your number
bool alertSent = false;

// Sensor Pins
const int trigPin = 9;
const int echoPin = 10;
const int flamePin = 8;
const int buzzerPin = 7;
const int rgbPin = 6;
const int gasPin = A0;

// Thresholds
const int MIN_DISTANCE = 30;
const int GAS_THRESHOLD = 300;
const int BUZZER_FREQ_ALERT = 1200;

void setup() {
  // Initialize components
  lcd.init();
  lcd.backlight();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(flamePin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(rgbPin, OUTPUT);
  pinMode(gasPin, INPUT);

  // GSM Module Initialization
  gsmSerial.begin(9600);
  delay(1000);
  sendATCommand("AT", 1000); // Check module
  sendATCommand("AT+CMGF=1", 1000); // Set SMS text mode
  sendATCommand("AT+CNMI=1,2,0,0,0", 1000); // Receive notifications

  // Gas sensor warm-up
  lcd.print("Calibrating...");
  delay(20000);
  lcd.clear();
}

void loop() {
  // Sensor readings
  float distance = getDistance();
  int flameState = digitalRead(flamePin);
  int gasValue = analogRead(gasPin);

  // Alert conditions
  bool objectNear = distance < MIN_DISTANCE;
  bool fireDetected = flameState == LOW;
  bool gasAlert = gasValue > GAS_THRESHOLD;

  // Priority alerts
  String statusMsg = "Safe";
  int rgbColor = 50;
  int buzzerFreq = 0;

  if(gasAlert) {
    statusMsg = "GAS LEAK!";
    rgbColor = 200;
    buzzerFreq = BUZZER_FREQ_ALERT;
    sendAlertSMS("GAS LEAK DETECTED!");
  }
  else if(fireDetected) {
    statusMsg = "FIRE!";
    rgbColor = 255;
    buzzerFreq = BUZZER_FREQ_ALERT;
    sendAlertSMS("FIRE DETECTED!");
  }
  else if(objectNear) {
    statusMsg = "OBJ NEAR!";
    rgbColor = 150;
    buzzerFreq = BUZZER_FREQ_ALERT;
    sendAlertSMS("OBJECT TOO CLOSE!");
  }
  else {
    alertSent = false; // Reset alert flag when safe
  }

  // Update outputs
  analogWrite(rgbPin, rgbColor);
  if(buzzerFreq > 0) tone(buzzerPin, buzzerFreq);
  else noTone(buzzerPin);

  // Update LCD
  lcd.setCursor(0, 0);
  lcd.print("D:");
  lcd.print(distance, 1);
  lcd.print("cm G:");
  lcd.print(gasValue);
  lcd.setCursor(0, 1);
  lcd.print("Status: ");
  lcd.print(statusMsg);

  delay(300);
}

float getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  return duration * 0.0343 / 2;
}

void sendAlertSMS(String message) {
  if(!alertSent) {
    sendATCommand("AT+CMGS=\"" + PHONE_NUMBER + "\"", 1000);
    gsmSerial.print(message);
    gsmSerial.write(26); // CTRL+Z to send
    alertSent = true;
    delay(1000); // Wait for SMS to send
  }
}

String sendATCommand(String command, int timeout) {
  String response = "";
  gsmSerial.println(command);
  delay(5);
  long endTime = millis() + timeout;
  while(millis() < endTime) {
    if(gsmSerial.available()) {
      response += (char)gsmSerial.read();
    }
  }
  return response;
}
