#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include <TM1637Display.h>
#include <Servo.h>

Servo myservo;

const char* ssid = "BigDom";
const char* password = "dpznrf21";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3 * 3600; // MSK = UTC+3
const int   daylightOffset_sec = 0;

constexpr uint8_t CLK_PIN = D2;
constexpr uint8_t DIO_PIN = D8;

constexpr uint8_t SERVO_PIN = D1;
constexpr uint8_t BUTTON_PIN = D5; // кнопка для выключения

TM1637Display display(CLK_PIN, DIO_PIN);

volatile bool servoEnabled = true; 
void IRAM_ATTR handleButton() {
    servoEnabled = !servoEnabled; // переключаем состояние при нажатии
}

void connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
}

void setupTime() {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void updateDisplay(const tm& t) {
    uint16_t value = t.tm_hour * 100 + t.tm_min;
    display.showNumberDecEx(value, 0b01000000, true);
}

void setup() {
    Serial.begin(115200);

    display.setBrightness(0x0f);
    display.clear();

    connectWiFi();
    setupTime();


    myservo.attach(SERVO_PIN);

    pinMode(BUTTON_PIN, INPUT_PULLUP); // подтяжка к питанию
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButton, FALLING);
}

void loop() {
    static uint32_t lastUpdate = 0;

    struct tm timeInfo;
    if (getLocalTime(&timeInfo)) {
        if (millis() - lastUpdate >= 1000) {
            lastUpdate = millis();
            updateDisplay(timeInfo);

            char buf[32];
            strftime(buf, sizeof(buf), "%H:%M:%S", &timeInfo);
            Serial.println(buf);
        }

        // с 21:10 до 21:35
        bool timeToRun = (timeInfo.tm_hour == 21 && timeInfo.tm_min >= 10 && timeInfo.tm_min <= 35);

        if (servoEnabled && timeToRun) {
            for (int pos = 0; pos <= 180; pos += 1) {
                myservo.write(pos);
                delay(10);
            }
            for (int pos = 180; pos >= 0; pos -= 1) {
                myservo.write(pos);
                delay(10);
            }
        } else {
            myservo.write(0);
        }
    }
}
