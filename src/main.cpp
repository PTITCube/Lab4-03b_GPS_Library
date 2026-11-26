/**
 * LAB 4 - VÍ DỤ 03b: THU NHẬN & PHÂN LOẠI BẢN TIN NMEA THÔ QUA THƯ VIỆN PTITCUBE
 */
#include <Arduino.h>
#include <PTITCube.h>

PTIT_CDH cdh;
PTIT_EPS eps;
PTIT_GPS gps;

void setup() {
    cdh.begin();

    // Kích hoạt hạ áp cấp nguồn GPS và ngoại vi qua thư viện EPS
    eps.setBuckState(true);

    gps.init();
}

void loop() {
    String sentence = gps.readSentence();
    if (sentence.length() > 0) {
        cdh.println(sentence);
        delay(2000);
    }
}
