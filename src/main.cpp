/**
 * LAB 4 - VÍ DỤ 03b: THU NHẬN & PHÂN LOẠI BẢN TIN NMEA THÔ QUA THƯ VIỆN PTITCUBE
 */
#include <Arduino.h>
#include <PTITCube.h>

PTIT_CDH cdh;
PTIT_GPS gps;

void setup() {
    cdh.begin(115200);
    gps.init(12, 14); // Khởi tạo GPS UART2 và chốt kênh MUX 74HC4066
}

void loop() {
    String sentence = gps.readSentence();
    if (sentence.length() > 0) {
        bool ok = gps.verifyChecksum(sentence.c_str());
        const char* type = gps.getNMEAType(sentence.c_str());

        cdh.printf("NMEA RAW: %-4s (%s) %s\n",
                   ok ? "OK" : "FAIL",
                   type,
                   sentence.c_str());

        delay(2000);
    }
}
