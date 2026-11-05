#include "Camera.h"
#include <SD.h>
#include <SPI.h>

namespace {
constexpr uint8_t SD_CS_PIN = 13;
constexpr uint8_t SD_SCK_PIN = 18;
constexpr uint8_t SD_MISO_PIN = 19;
constexpr uint8_t SD_MOSI_PIN = 23;

// Baudrate giao tiếp với ESP32-CAM (UART2 của CAM ở 115200)
constexpr uint32_t CAM_BAUD = 115200;
// Baudrate giao tiếp với GPS (9600)
constexpr uint32_t GPS_BAUD = 9600;
}

PTIT_Camera::PTIT_Camera() : gpsSelectPin(12), camSelectPin(14), sdReady(false) {
}

bool PTIT_Camera::initializeSd() {
    // Nếu thẻ nhớ đã khởi tạo thành công trước đó, không cần khởi tạo lại
    if (sdReady) return true;
    
    Serial.println("[Camera] Khoi tao MicroSD qua SPI...");
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    if (!SD.begin(SD_CS_PIN, SPI, 8000000)) {
        Serial.println("[Camera] MicroSD mount failed");
        sdReady = false;
        return false;
    }

    sdReady = true;
    Serial.println("[Camera] MicroSD ready for image storage");
    return true;
}

bool PTIT_Camera::init(uint8_t gpsSelectPin, uint8_t camSelectPin) {
    this->gpsSelectPin = gpsSelectPin;
    this->camSelectPin = camSelectPin;

    if (gpsSelectPin != 255 && camSelectPin != 255) {
        pinMode(this->gpsSelectPin, OUTPUT);
        pinMode(this->camSelectPin, OUTPUT);
        digitalWrite(this->gpsSelectPin, LOW);
        digitalWrite(this->camSelectPin, HIGH); // Mặc định mở kênh Camera
    }

    // ESP32-CAM giờ dùng UART2 (GPIO 14 TX / GPIO 15 RX)
    // ESP32 chính giao tiếp qua UART2 (GPIO 16 RX / GPIO 17 TX)
    Serial.println("[Camera] Khoi tao giao tiep voi ESP32-CAM (CAM_UART2 <-> OBC_UART2)...");
    Serial2.begin(CAM_BAUD, SERIAL_8N1, 16, 17);

    return initializeSd();
}

void PTIT_Camera::selectGPS() {
    if (gpsSelectPin != 255 && camSelectPin != 255) {
        digitalWrite(gpsSelectPin, HIGH);
        digitalWrite(camSelectPin, LOW);
        // Trả lại cấu hình UART2 cho GPS (baudrate 9600)
        Serial2.begin(GPS_BAUD, SERIAL_8N1, 16, 17);
    }
}

void PTIT_Camera::selectCamera() {
    if (gpsSelectPin != 255 && camSelectPin != 255) {
        digitalWrite(gpsSelectPin, LOW);
        digitalWrite(camSelectPin, HIGH);
        // Đặt lại cấu hình UART2 cho Camera (baudrate 115200)
        Serial2.begin(CAM_BAUD, SERIAL_8N1, 16, 17);
    }
}

bool PTIT_Camera::isReady() const {
    return sdReady;
}

bool PTIT_Camera::capture() {
    return captureToFile("/photos/capture.jpg");
}

bool PTIT_Camera::captureToFile(const char* path) {
    if (!sdReady) {
        Serial.println("[Camera] Loi: The nho SD chua san sang.");
        return false;
    }

    // Đảm bảo kênh MUX đang trỏ vào Camera
    selectCamera();

    // Dọn dẹp bộ nhớ đệm
    while(Serial2.available()) Serial2.read();

    // 1. Gửi lệnh CAPTURE đến ESP32-CAM qua UART2
    Serial.println("[Camera] Dang gui lenh chup anh toi ESP32-CAM...");
    Serial2.print("CAPTURE\n");
    Serial2.flush();

    // 2. Chờ ESP32-CAM phản hồi về SIZE
    String response = "";
    unsigned long startTime = millis();
    bool sizeReceived = false;
    long imageSize = 0;

    while (millis() - startTime < 5000) {
        if (Serial2.available()) {
            response = Serial2.readStringUntil('\n');
            response.trim();
            if (response.startsWith("SIZE:")) {
                imageSize = response.substring(5).toInt();
                sizeReceived = true;
                break;
            }
            if (response.startsWith("ERROR:")) {
                Serial.println("[Camera] ESP32-CAM: " + response);
                return false;
            }
        }
        delay(1);
    }

    if (!sizeReceived || imageSize <= 0) {
        Serial.println("[Camera] Loi: Khong nhan duoc phan hoi kich thuoc tu ESP32-CAM.");
        return false;
    }

    Serial.printf("[Camera] ESP32-CAM phan hoi kich thuoc anh: %ld bytes\n", imageSize);

    // 3. Tạo thư mục nếu chưa có
    String filePath = path;
    int lastSlash = filePath.lastIndexOf('/');
    if (lastSlash > 0) {
        String dirPath = filePath.substring(0, lastSlash);
        if (!SD.exists(dirPath.c_str())) {
            if (!SD.mkdir(dirPath.c_str())) {
                Serial.println("[Camera] Loi: Khong the tao thu muc " + dirPath);
                return false;
            }
        }
    }

    // 4. Mở file để ghi
    File file = SD.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("[Camera] Loi: Khong the tao file " + String(path));
        return false;
    }

    // 5. Báo cho ESP32-CAM biết OBC đã sẵn sàng nhận dữ liệu
    Serial2.print("OK\n");
    Serial2.flush();
    Serial.println("[Camera] Dang tai du lieu anh tu ESP32-CAM...");

    // 6. Nhận dữ liệu raw từ UART2 và ghi trực tiếp vào thẻ nhớ
    long receivedBytes = 0;
    startTime = millis();
    uint8_t buf[256];

    while (receivedBytes < imageSize && (millis() - startTime < 15000)) {
        if (Serial2.available()) {
            int bytesToRead = Serial2.available();
            if (bytesToRead > (int)sizeof(buf)) bytesToRead = sizeof(buf);
            
            int bytesRead = Serial2.readBytes(buf, bytesToRead);
            file.write(buf, bytesRead);
            receivedBytes += bytesRead;
            
            // Reset timeout mỗi khi nhận được dữ liệu
            startTime = millis();
        }
    }

    file.close();

    // 7. Hoàn tất
    if (receivedBytes == imageSize) {
        Serial.println("[Camera] Luu anh thanh cong: " + String(path));
        Serial2.println("DONE"); // Báo ESP32-CAM kết thúc
        return true;
    } else {
        Serial.printf("[Camera] Loi: Chi nhan duoc %ld/%ld bytes. Timeout!\n", receivedBytes, imageSize);
        return false;
    }
}
