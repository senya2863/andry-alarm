#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include <TM1637Display.h>
#include <Servo.h>

Servo myservo;
bool servoEnabled = true;

struct ServoState {
    int pos = 0;
    int step = 1;           // направление движения
    unsigned long lastMove = 0;
    unsigned long interval = 10;  // миллисекунды между шагами
} servoState;

const char* ssid = "BigDom"; //название сети
const char* password = "dpznrf21"; //пароль

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3 * 3600; // MSK = UTC+3
const int   daylightOffset_sec = 0;

constexpr uint8_t CLK_PIN = D2;
constexpr uint8_t DIO_PIN = D8;

constexpr uint8_t SERVO_PIN = D1;
constexpr uint8_t BUTTON_PIN = D5; // кнопка для выключения

TM1637Display display(CLK_PIN, DIO_PIN);

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
    static unsigned long lastDisplayUpdate = 0;
    struct tm timeInfo;

    if (!getLocalTime(&timeInfo)) return;

    unsigned long now = millis();

    // --- Обновление дисплея раз в секунду ---
    if (now - lastDisplayUpdate >= 1000) {
        lastDisplayUpdate = now;
        updateDisplay(timeInfo);

        char buf[32];
        strftime(buf, sizeof(buf), "%H:%M:%S", &timeInfo);
        Serial.println(buf);
    }

//время будильника
    bool timeToRun = (timeInfo.tm_hour == 21 && timeInfo.tm_min >= 10 && timeInfo.tm_min <= 35);

    if (servoEnabled && timeToRun) {
        // --- Non-blocking движение сервы ---
        if (now - servoState.lastMove >= servoState.interval) {
            servoState.lastMove = now;

            // Двигаем серву
            myservo.write(servoState.pos);

            // Обновляем позицию
            servoState.pos += servoState.step;

            // Разворот направления на границах
            if (servoState.pos >= 180) servoState.step = -1;
            if (servoState.pos <= 0)   servoState.step = 1;
        }
    } else {
        // Если время не подошло, держим серву в 0
        myservo.write(0);
        servoState.pos = 0;
        servoState.step = 1;
    }
}
