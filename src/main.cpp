/**
 * LAB 4 - VÍ DỤ 03b: THU NHẬN & HIỂN THỊ BẢN TIN NMEA THÔ QUA BỘ GIẢI MÃ DÒNG
 * 
 * Mục tiêu:
 * - Thay vì in từng ký tự rời rạc như ở ví dụ 03a, ví dụ này sử dụng kỹ thuật
 *   bộ đệm dòng (Line Buffering) và xử lý luồng NMEA để thu nhận trọn vẹn từng câu tin.
 * - Đầu ra tiếp tục hiển thị dữ liệu NMEA THÔ (Raw NMEA), nhưng được phân loại câu tin
 *   ($GNRMC, $GNGGA, $GNVTG, $GNGSV) và kiểm tra tính toàn vẹn bằng mã Checksum XOR (*hh).
 * - Giúp người học quan sát rõ cấu trúc chi tiết của từng câu tin NMEA thô chuẩn quốc tế NMEA 0183.
 */

#include <Arduino.h>

#include <PTITCube.h>

PTIT_CDH cdh;

#define GPS_RX 16
#define GPS_TX 17
#define GPS_BAUD 9600

#define MUX_GPS_PIN 12
#define MUX_CAM_PIN 14

// Bộ đệm lưu trữ một câu tin NMEA hoàn chỉnh (chuẩn NMEA tối đa 82 ký tự)
static char nmeaBuffer[128];
static uint8_t bufferIndex = 0;

// Thống kê số lượng bản tin NMEA thô
static uint32_t totalSentences = 0;
static uint32_t validChecksumCount = 0;
static uint32_t errorChecksumCount = 0;

// Hàm tính và kiểm tra Checksum XOR của một câu tin NMEA thô
// Checksum NMEA được tính bằng XOR tất cả các ký tự nằm giữa '$' và '*'
bool verifyNMEAChecksum(const char* sentence) {
    if (sentence[0] != '$') return false;

    uint8_t calculatedChecksum = 0;
    int i = 1;

    // Tính XOR cho tới khi gặp '*' hoặc kết thúc chuỗi
    while (sentence[i] != '*' && sentence[i] != '\0') {
        calculatedChecksum ^= (uint8_t)sentence[i];
        i++;
    }

    if (sentence[i] != '*') return false; // Không tìm thấy ký tự '*'

    // Đọc mã Checksum 2 ký tự Hex sau dấu '*'
    char hexStr[3] = { sentence[i + 1], sentence[i + 2], '\0' };
    uint8_t receivedChecksum = (uint8_t)strtol(hexStr, NULL, 16);

    return (calculatedChecksum == receivedChecksum);
}

// Hàm nhận diện và phân loại câu tin NMEA thô
const char* getNMEAType(const char* sentence) {
    if (strstr(sentence, "RMC")) return "RMC (Recommended Minimum)";
    if (strstr(sentence, "GGA")) return "GGA (Global Positioning Fix)";
    if (strstr(sentence, "VTG")) return "VTG (Track Made Good & Speed)";
    if (strstr(sentence, "GSA")) return "GSA (DOP & Active Satellites)";
    if (strstr(sentence, "GSV")) return "GSV (Satellites in View)";
    if (strstr(sentence, "GLL")) return "GLL (Geographic Position)";
    return "OTHER";
}

void processNMEASentence(const char* sentence) {
    totalSentences++;
    bool isCheckOk = verifyNMEAChecksum(sentence);

    if (isCheckOk) {
        validChecksumCount++;
    } else {
        errorChecksumCount++;
    }

    // In câu tin NMEA THÔ kèm nhãn phân loại và trạng thái kiểm tra Checksum
    cdh.printf("[NMEA RAW] [%-4s] [%s] %s\n",
                  isCheckOk ? "OK" : "FAIL",
                  getNMEAType(sentence),
                  sentence);
}

void setup() {
    cdh.begin(115200);
    while (!cdh) { delay(10); }

    // Chốt mức logic MUX 74HC4066: Ngắt Camera, Mở GPS
    pinMode(MUX_GPS_PIN, OUTPUT);
    pinMode(MUX_CAM_PIN, OUTPUT);
    digitalWrite(MUX_CAM_PIN, LOW);
    delayMicroseconds(50);
    digitalWrite(MUX_GPS_PIN, HIGH);

        cdh.println("  PTITCUBE - LAB 4-03b: BẢN TIN NMEA THÔ & CHECKSUM ");
        cdh.println("[INFO] Khởi tạo bộ thu bản tin NMEA thô qua UART2...");
    cdh.println("[INFO] Hệ thống sẽ thu nhận trọn vẹn từng câu tin thô và kiểm tra mã Checksum XOR.");
    cdh.println("==================================================\n");

    Serial2.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);
}

void loop() {
    // Đọc luồng byte từ GPS và gom thành từng câu tin hoàn chỉnh
    while (Serial2.available() > 0) {
        char c = (char)Serial2.read();

        if (c == '$') {
            // Bắt đầu một câu tin NMEA mới
            bufferIndex = 0;
            nmeaBuffer[bufferIndex++] = c;
        } else if (c == '\n' || c == '\r') {
            // Kết thúc câu tin NMEA
            if (bufferIndex > 0 && nmeaBuffer[0] == '$') {
                nmeaBuffer[bufferIndex] = '\0'; // Đóng chuỗi C-string
                processNMEASentence(nmeaBuffer);
                bufferIndex = 0;
            }
        } else {
            // Lưu ký tự vào bộ đệm nếu chưa tràn
            if (bufferIndex < sizeof(nmeaBuffer) - 1) {
                nmeaBuffer[bufferIndex++] = c;
            }
        }
    }

    // In thống kê định kỳ mỗi 5 giây
    static unsigned long lastStats = 0;
    if (millis() - lastStats >= 5000) {
        lastStats = millis();
        if (totalSentences > 0) {
                        cdh.printf("[THỐNG KÊ NMEA THÔ] Tổng số câu: %u | Checksum hợp lệ: %u | Lỗi: %u\n",
                          totalSentences, validChecksumCount, errorChecksumCount);
                    }
    }
}
