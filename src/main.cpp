/**
 * LAB 4 - VÍ DỤ 03b: THU NHẬN & HIỂN THỊ BẢN TIN NMEA THÔ QUA BỘ GIẢI MÃ DÒNG
 */
#include <Arduino.h>
#include <PTITCube.h>

PTIT_CDH cdh;

#define GPS_RX 16
#define GPS_TX 17
#define GPS_BAUD 9600

#define MUX_GPS_PIN 12
#define MUX_CAM_PIN 14

static char nmeaBuffer[128];
static uint8_t bufferIndex = 0;

// Hàm tính và kiểm tra Checksum XOR của một câu tin NMEA thô
bool verifyNMEAChecksum(const char* sentence) {
    if (sentence[0] != '$') return false;

    uint8_t calculatedChecksum = 0;
    int i = 1;

    while (sentence[i] != '*' && sentence[i] != '\0') {
        calculatedChecksum ^= (uint8_t)sentence[i];
        i++;
    }

    if (sentence[i] != '*') return false;

    char hexStr[3] = { sentence[i + 1], sentence[i + 2], '\0' };
    uint8_t receivedChecksum = (uint8_t)strtol(hexStr, NULL, 16);

    return (calculatedChecksum == receivedChecksum);
}

// Hàm nhận diện và phân loại câu tin NMEA thô
const char* getNMEAType(const char* sentence) {
    if (strstr(sentence, "RMC")) return "RMC";
    if (strstr(sentence, "GGA")) return "GGA";
    if (strstr(sentence, "VTG")) return "VTG";
    if (strstr(sentence, "GSA")) return "GSA";
    if (strstr(sentence, "GSV")) return "GSV";
    if (strstr(sentence, "GLL")) return "GLL";
    return "OTHER";
}

void processNMEASentence(const char* sentence) {
    bool isCheckOk = verifyNMEAChecksum(sentence);

    cdh.printf("NMEA RAW: %-4s (%s) %s\n",
               isCheckOk ? "OK" : "FAIL",
               getNMEAType(sentence),
               sentence);
}

void setup() {
    cdh.begin(115200);

    // Chốt mức logic MUX 74HC4066: Ngắt Camera, Mở GPS
    pinMode(MUX_GPS_PIN, OUTPUT);
    pinMode(MUX_CAM_PIN, OUTPUT);
    digitalWrite(MUX_CAM_PIN, LOW);
    delayMicroseconds(50);
    digitalWrite(MUX_GPS_PIN, HIGH);

    Serial2.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);
}

void loop() {
    while (Serial2.available() > 0) {
        char c = (char)Serial2.read();

        if (c == '$') {
            bufferIndex = 0;
            nmeaBuffer[bufferIndex++] = c;
        } else if (c == '\n' || c == '\r') {
            if (bufferIndex > 0 && nmeaBuffer[0] == '$') {
                nmeaBuffer[bufferIndex] = '\0';
                processNMEASentence(nmeaBuffer);
                bufferIndex = 0;
                delay(2000);
                break;
            }
        } else {
            if (bufferIndex < sizeof(nmeaBuffer) - 1) {
                nmeaBuffer[bufferIndex++] = c;
            }
        }
    }
}
