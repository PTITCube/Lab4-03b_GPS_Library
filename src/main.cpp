/**
 * VÍ DỤ 2: KIỂM TRA PHÂN HỆ ĐỊNH VỊ VỆ TINH (GPS MODULE)
 * 
 * Mô tả:
 * Ví dụ này minh họa cách giao tiếp với module GPS (như dòng NEO-xM, L78x, L80) 
 * để lấy tọa độ địa lý, độ cao, tốc độ, số vệ tinh kết nối và thời gian thực chuẩn UTC.
 */

#include <Arduino.h>
#include <PTITCube.h>

// 1. KHAI BÁO ĐỐI TƯỢNG
PTIT_GPS myGPS;

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }

    Serial.println("\n[EXAMPLE] Bắt đầu bài test Phân hệ Định vị (GPS)...");
    myGPS.init();
    Serial.println("[EXAMPLE] Đang chờ fix tín hiệu từ vệ tinh...");
}

void loop() {
    // Luôn luôn gọi hàm update() trong vòng lặp để thu thập NMEA sentences
    myGPS.update();

    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 3000) {
        lastPrint = millis();
        
        Serial.println("-------------------------------------------------");
        int sats = myGPS.getSatellites();
        if (sats == 0) {
            Serial.println("[WARNING] Đang quét tìm vệ tinh... Hãy đem mạch ra ngoài trời.");
            return;
        }

        float lat = myGPS.getLat();
        float lng = myGPS.getLng();
        
        if (lat == 0.0 && lng == 0.0) {
            Serial.println("[WARNING] Đã thấy vệ tinh nhưng chưa khóa được tọa độ.");
        } else {
            Serial.println(">>> THÔNG TIN ĐỊNH VỊ (GPS) <<<");
            Serial.printf("    - Tọa độ (Vĩ độ, Kinh độ) : %f, %f\n", lat, lng);
            Serial.printf("    - Cao độ so với mặt biển  : %.2f m\n", myGPS.getAltitude());
            Serial.printf("    - Tốc độ di chuyển        : %.2f km/h\n", myGPS.getSpeed());
            Serial.printf("    - Hướng di chuyển (Course): %.2f độ\n", myGPS.getCourse());
            Serial.printf("    - Số vệ tinh khóa được    : %d\n", sats);
            Serial.printf("    - Độ nhiễu vị trí (HDOP)  : %.2f\n", myGPS.getHDOP());
            Serial.print ("    - Thời gian chuẩn (UTC)   : ");
            Serial.println(myGPS.getTimeString());
        }
        Serial.println("-------------------------------------------------");
    }
}
