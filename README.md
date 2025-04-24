# Smart Gas Leak Detection System

sistem berbasis IoT yang dirancang untuk mendeteksi kebocoran gas LPG di lingkungan dapur industri. Sistem ini menggunakan mikrokontroler ESP32 dan sensor gas MQ-2 untuk memantau kadar gas, memberikan peringatan real-time melalui bot Telegram saat kebocoran terdeteksi. Sistem juga dilengkapi dengan kontrol kipas pembuangan otomatis untuk mengurangi penumpukan gas selama kejadian kebocoran.

## Fitur Utama
- Deteksi kebocoran gas real-time dengan sensor MQ-2
- Integrasi bot Telegram untuk pemantauan dan kontrol jarak jauh
- Aktivasi kipas pembuangan otomatis saat kebocoran dengan kontrol kecepatan PWM
- Pengaturan dual-sensor untuk cakupan dapur yang komprehensif
- Alarm buzzer untuk peringatan lokal
- Implementasi FreeRTOS untuk pengelolaan tugas yang efisien

## Komponen Perangkat Keras
- ESP32 Development Board
- 2 × Sensor Gas MQ-2
- Buzzer untuk peringatan suara
- Motor DC (Kipas Pembuangan) dengan Driver Motor L298N
- Catu daya

## Konfigurasi Pin
| Komponen         | Nomor Pin  |
|------------------|------------|
| Sensor MQ-2 1    | GPIO 33    |
| Sensor MQ-2 2    | GPIO 32    |
| Buzzer           | GPIO 27    |
| Motor Driver IN1 | GPIO 12    |
| Motor Driver IN2 | GPIO 14    |
| Motor Driver ENA | GPIO 13    |

## Dependensi Perangkat Lunak
- Arduino Framework untuk ESP32
- FreeRTOS untuk manajemen tugas
- Konektivitas WiFi
- Library ArduinoJson
- Library UniversalTelegramBot

## Petunjuk Pengaturan

### Pengaturan Perangkat Keras
1. Hubungkan sensor MQ-2 ke pin 33 (Dapur A) dan 32 (Dapur B)
2. Hubungkan buzzer ke pin 27
3. Hubungkan driver motor L298N:
   - IN1 ke pin 12
   - IN2 ke pin 14
   - ENA ke pin 13

### Pengaturan Perangkat Lunak
1. Instal PlatformIO IDE (direkomendasikan) atau Arduino IDE dengan dukungan ESP32
2. Clone repository ini
3. Perbarui konfigurasi berikut di `src/main.cpp`:
   - Kredensial WiFi (`WIFI_SSID` dan `WIFI_PASSWORD`)
   - Token Bot Telegram (`BOT_TOKEN`)
   - ID Chat Telegram (`CHAT_ID`)
   - URL kamera jika menggunakan integrasi ESP32-CAM
4. Build dan upload kode ke ESP32 Anda

## Perintah Bot Telegram
- `/start` - Pesan selamat datang dan daftar perintah yang tersedia
- `/streaming` - Mendapatkan URL untuk streaming video langsung (jika kamera dikonfigurasi)
- `/turnon_exhaust_fan_pwm100` - Menyalakan kipas pembuangan pada kecepatan sedang (PWM 100)
- `/turnoff_exhaust_fan` - Mematikan kipas pembuangan
- `/about` - Informasi tentang sistem dan fitur-fiturnya

## Operasi Sistem
1. Sistem terus memantau kadar gas melalui dua sensor MQ-2
2. Jika konsentrasi gas melebihi ambang batas (>400 untuk Sensor 1 atau >1500 untuk Sensor 2):
   - Alarm buzzer diaktifkan
   - Peringatan Telegram dikirim dengan detail konsentrasi gas
   - Kipas pembuangan diaktifkan secara otomatis pada kecepatan maksimum (PWM 255)
3. Pengguna dapat mengontrol kipas pembuangan secara manual melalui perintah Telegram

## Arsitektur Proyek
Proyek ini menggunakan FreeRTOS untuk mengelola beberapa tugas bersamaan:
- `taskSensorA` - Membaca data dari sensor gas 1
- `taskSensorB` - Membaca data dari sensor gas 2
- `taskLeakDetection` - Memproses data sensor untuk mendeteksi kebocoran dan memicu peringatan
- `taskTelegram` - Menangani komunikasi bot Telegram
- `keepWiFiAlive` - Memastikan konektivitas WiFi tetap terjaga

## Kustomisasi
- Ambang batas sensor dapat disesuaikan dalam fungsi `taskLeakDetection`
- Nilai PWM untuk kecepatan kipas dapat dimodifikasi sesuai kebutuhan
- Perintah Telegram tambahan dapat diimplementasikan dalam fungsi `handleNewMessages`