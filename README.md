# Lab4-03b_GPS_Library

**Thu nhận & Phân loại Bản tin NMEA Thô với Kiểm tra Checksum**

Bài thực hành thuộc Phân hệ Cảm biến & Định vị (Lab 4) trong hệ thống PTITCube.

## 1. Mục tiêu
- Thu nhận các chuỗi bản tin NMEA thô từ module GPS Quectel L76X bằng kỹ thuật bộ đệm dòng (Line Buffering).
- Nhận diện và phân loại các loại câu tin NMEA thô phổ biến:
  - `$GNRMC` / `$GPRMC`: Recommended Minimum Specific GNSS Data.
  - `$GNGGA` / `$GPGGA`: Global Positioning System Fix Data.
  - `$GNVTG` / `$GPVTG`: Track Made Good and Ground Speed.
  - `$GNGSA` / `$GPGSA`: GNSS DOP and Active Satellites.
  - `$GNGSV` / `$GPGSV`: GNSS Satellites in View.
- Cài đặt thuật toán kiểm tra tính toàn vẹn **Checksum XOR** (`*hh`) của chuẩn NMEA 0183 để phát hiện lỗi đường truyền UART.
- Đầu ra hiển thị dữ liệu NMEA thô có cấu trúc và thống kê tỉ lệ bản tin hợp lệ.

## 2. Phần cứng sử dụng
- **ESP32 NodeMCU OBC**
- **Module định vị GNSS:** Quectel L76X.
- **IC chuyển mạch:** 74HC4066 (chân chọn GPS: GPIO 12, chân chọn Camera: GPIO 14).
- **Cổng giao tiếp:** UART2 (RX2 = GPIO 16, TX2 = GPIO 17, Baudrate: `9600`).
- **Serial Monitor:** USB UART (Baudrate: `115200`).
