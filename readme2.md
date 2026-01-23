# QMS-DISPLAY NUMBER SCREEN

ESP32-S3 + 7 Inch LCD Touch Screen

Thiết bị màn hình hiển thị số thứ tự khách hàng sử dụng **ESP32-S3** và **LCD TFT 7 inch**,  
ứng dụng cho ngân hàng, bệnh viện, trung tâm hành chính, quầy giao dịch.

Khách hàng có thể biết được số thứ tự được gọi đến quày giao dịch.

---

## ✨ Features

- Giao diện hiển thị số thứ tự khách hàng tại quầy trực quan trên LCD 7 inch
- Hỗ trợ cảm ứng
- Các chế độ hiển thị:
  - Hiển thị số thứ tự khách hàng
  - Hiển thị dòng chữ "QUẦY TẠM THỜI ĐÓNG"
  - Khi reset thì màn hình trống, không hiển thị
- Các giao diện của màn hình gồm có:
  - Giao diện hiển thị số đang gọi hoặc hiển thị dòng chữ báo đóng quầy (Screen 1)
  - Giao diện menu (Screen 2)
  - Giao diện chọn keypad để kết nối đến (Screen 3)
  - Giao diện đăng nhập wifi (Wifi Screen)

- Kết nối tới WiFi được nhập từ màn hình
- Wifi sau khi được kết nối thành công sẽ được lưu trong bộ nhớ NVS và tự động kết nối khi khởi động thiết bị
- Kết nối lại wifi 10 lần khi bị mất kết nối
- Hiển biểu tượng sóng wifi 5 mức độ trên giao diện đánh giá và đăng nhập để người dùng biết tình trạng kết nối wifi
- Kết nối đến server và đến bàn phím gọi số tại quầy thông qua MQTT
- Có thể lựa chọn bàn phím gọi số tại quầy mà người dùng muốn kết nối đến, kết quả sẽ được lưu trong bộ nhớ NVS cho các lần sử dụng tiếp theo
- Hiển thị số thứ tự khách hàng nhận được từ bàn phím gọi số
- Lưu lại số thứ tự khách hàng hiện tại trong bộ nhớ NVS và hiển thị lại số đó trong lần khởi động tiếp theo cho trường hợp có sự cố mất điện

---

## 🧩 System Overview

- MCU: **ESP32-S3**
- Display: **LCD TFT 7 inch**
- Touch Panel: Capacitive
- Connectivity: WiFi
- Storage: Flash nội ESP32

---

## 🔌 Hardware

| Thành phần            | Mô tả          |
| --------------------- | -------------- |
| MCU                   | ESP32-S3       |
| LCD                   | TFT 7 inch     |
| Độ phân giải màn hình | 1024 × 600     |
| Touch Panel           | Capacitive     |
| Flash                 | Internal Flash |
| Nguồn                 | 5V             |

---

## 📁 Project Structure

```
project/
├── main/
│ ├── esp_mqtt_client/
│ └── user/
|      └──wifi
└── README.md
```

## 🔄 System Workflow

Các nhóm chức năng của thiết bị

### 1. Boot & reconnect

- Thiết bị khởi động.
- Hiển thị Screen 1 (màn hình chính)
- Kiểm tra thông tin wifi trong bộ nhớ, nếu có lưu trước đó → Tự động kết nối đến wifi này
- Nếu kết nối wifi thành công thì kết nối đến server, nếu không thành công sẽ retry **5 lần**
- Trạng thái kết nối đến wifi được hiển thị thông qua icon cột sóng wifi ở góc phải trên cùng màn hình
- Nếu thiết bị kết nối đến server thất bại → Hiển thị icon mất kết nối tại góc phải dưới cùng màn hình
- Nếu thiết bị kết nối đến server thành công → Không hiển thị icon
- Chạm **3 lần liên tục góc trên cùng bên trái** → vào giao diện menu (Screen 2)

---

### 2. Update device status

- Gửi tin nhắn báo **online** lên server qua **MQTT** khi kết nối thành công
- Cấu hình last will để Khi mất kết nối đến server trong vòng 25s thì server sẽ cập nhật trạng thái thiết bị lúc là **offline**

---

### 3. Display

- Thiết bị nhận được số thứ tự khách hàng từ bàn phím gọi số → hiển thị màn hình số thứ tự
- Thiết bị nhận được lệnh reset từ bàn phím gọi số → màn hình hiển thị **Chúng tôi vô cùng cảm ơn phản hồi của quý khách!** trong 1 giây
- Khi chọn "RESET" tại màn hình menu thì màn hình hiển thị trống

---

### 4. Menu & Settings

- Ở màn hình chính:
  - Chạm **3 lần góc trên bên trái** → vào menu để chọn chức năng
- Các chức năng:
  - **WIFI**:
    - Vào màn hình cấu hình wifi
  - **KEYPAD**:
    - Chọn bàn phím gọi số trong hệ thống
    - Thiết bị được chọn hiển thị số màu đen
  - **RESET**:
    - Xóa số hiển thị hiện tại trong bộ nhớ, màn hình hiển thị số lúc này sẽ trống

### 3. WiFi Configuration

- Giao diện WiFi:
  - `Refresh`: quét lại danh sách WiFi
  - `Back`: quay lại màn hình trước
  - `Switch`: bật / tắt WiFi
- Khi khởi động:
  - Không hiển thị danh sách WiFi cho đến khi switch được bật
  - Tự động kết nối WiFi đã lưu trong NVS

---

### 6. Select keypad

- Thiết bị nhận được list các bàn phím gọi số trong hệ thống từ server thông qua MQTT
- List các thiết bị hiển thị trong Screen3
- Keypad được chọn sẽ có **tên được in đen** tại list button

---
