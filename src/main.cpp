#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>

// Konfirgurasi PIN
#define GAS_SENSOR_PIN_1 33 // Pin GPIO untuk Sensor MQ-2 Dapur 1
#define GAS_SENSOR_PIN_2 32 // Pin GPIO untuk Sensor MQ-2 Dapur 2
#define BUZZER_PIN 27       // Pin GPIO untuk Buzzer
#define IN1 12              // deklarasi pin IN1
#define IN2 14              // deklarasi pin IN2
#define ENA 13              // deklarasi pin ENA

// Camera URL
String camURL = "127.0.0.1";

// Konfigurasi WiFi
#define WIFI_SSID "test"
#define WIFI_PASSWORD "bismillah123"
#define WIFI_TIMEOUT_MS 20000

// Konfirgurasi Telegram
#define BOT_TOKEN "6803152509:AAGZ1VGCjvadvooIcBVwoBTv-YEHNlgEY0Q"
#define CHAT_ID "-4038096796"

// Cek pesan baru setiap 0,5 detik.
int botRequestDelay = 500;
unsigned long lastTimeBotRan;

// Init
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

// FreeRTOS Task Handles
TaskHandle_t TaskSensorAHandle = NULL;
TaskHandle_t TaskSensorBHandle = NULL;
TaskHandle_t TaskTelegramHandle = NULL;
TaskHandle_t TaskLeakDetectionHandle = NULL;

// Mutex
SemaphoreHandle_t xBotMutex;

// Buffer untuk menyimpan data sensor
int gasValue1;
int gasValue2;

// Task untuk membaca data dari sensor Gas 1
void taskSensorA(void *pvParameters);

// Task untuk membaca data dari sensor Gas 2
void taskSensorB(void *pvParameters);

// Task untuk mendeteksi kebocoran
void taskLeakDetection(void *pvParameters);

// untuk mengupdate nilai sensor 1
boolean updateGasSensorA();

// untuk mengupdate nilai sensor 2
boolean updateGasSensorB();

// Task untuk menghandle request dari bot telegram
void taskTelegram(void *pvParameters);

// Menghandle ketika ada pesan baru
void handleNewMessages(int numNewMessages);

void keepWiFiAlive(void *pvParameters);

void turnonfan(int pwnSpeed);
void turnofffan();

void setup()
{
    Serial.begin(115200);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    WiFi.disconnect();                                   // Untuk memastikan state WiFi disconnect diawal
    secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Konfigurasi SSL diperlukan untuk menggunakan UniversalTelegramBot

    randomSeed(analogRead(0));

    // Setup PIN
    pinMode(GAS_SENSOR_PIN_1, INPUT);
    pinMode(GAS_SENSOR_PIN_2, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(ENA, OUTPUT);

    // // State awal motor mati
    digitalWrite(ENA, LOW);

    // Setup Mutex
    xBotMutex = xSemaphoreCreateMutex();

    // Task RTOS
    xTaskCreatePinnedToCore(keepWiFiAlive, "Task Keep Wifi Alive", 5120, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(taskSensorA, "Task Sensor A", 1024, NULL, 1, &TaskSensorAHandle, 0);
    xTaskCreatePinnedToCore(taskSensorB, "Task Sensor B", 1024, NULL, 1, &TaskSensorBHandle, 0);
    xTaskCreatePinnedToCore(taskLeakDetection, "Task Leak Detection", 8192, NULL, 1, &TaskLeakDetectionHandle, 0);
    xTaskCreatePinnedToCore(taskTelegram, "Task Telegram", 8192, NULL, 1, &TaskTelegramHandle, 1);
}

void loop()
{
    // Kosong karena penggunaan FreeRTOS
}

void taskSensorA(void *pvParameters)
{
    for (;;)
    {
        // ESP_LOGI("Task Sensor A", "Running");

        while (!updateGasSensorA())
        {
            ESP_LOGI("DHT", "DHT error! retry in 3 seconds");
            vTaskDelay(3000 / portTICK_PERIOD_MS);
        }

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void taskSensorB(void *pvParameters)
{
    for (;;)
    {
        // ESP_LOGI("Task Sensor B", "Running");

        while (!updateGasSensorB())
        {
            ESP_LOGI("BH1750", "BH1750 error! retry in 3 seconds");
            vTaskDelay(3000 / portTICK_PERIOD_MS);
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void taskLeakDetection(void *pvParameters)
{
    // ESP_LOGI("Task Leak Detection", "Running");
    for (;;)
    {
        if (xSemaphoreTake(xBotMutex, portMAX_DELAY) == pdTRUE)
        {
            // ESP_LOGI("gasValue1", "Nilai sensor gas 1: %i", gasValue1);
            // ESP_LOGI("gasValue2", "Nilai sensor gas 2: %i", gasValue2);

            if (gasValue1 > 400)
            {
                ESP_LOGI("Dapur 1", "Terdeteksi Kebocoran Gas LPG");

                float gasConcentration1 = map(gasValue1, 0, 4095, 0, 100);
                String message = "Terdeteksi Kebocoran Gas LPG di Dapur A\n";
                message += "Kadar Gas: " + String(gasConcentration1) + "%\n";
                message += "Periksa Dapur Anda Sekarang Juga!!!.\n\n";
                message += "Exhaust Fan Menyala dengan Kecepatan Maksimal di PWM 255.\n";

                digitalWrite(BUZZER_PIN, HIGH); // Aktifkan buzzer
                bot.sendMessage(CHAT_ID, message, "");
                turnonfan(255);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
            else
            {
                digitalWrite(BUZZER_PIN, LOW);
                ESP_LOGI("Dapur 1", "Tidak ada kebocoran gas");
            }

            if (gasValue2 > 1500)
            {
                ESP_LOGI("Dapur 2", "Terdeteksi Kebocoran Gas LPG");

                float gasConcentration2 = map(gasValue2, 0, 4095, 0, 100);
                String message = "Terdeteksi Kebocoran Gas LPG di Dapur B\n";
                message += "Kadar Gas: " + String(gasConcentration2) + "%\n";
                message += "Periksa Dapur Anda Sekarang Juga!!!.\n\n";
                message += "Exhaust Fan Menyala dengan Kecepatan Maksimal di PWM 255.\n";

                digitalWrite(BUZZER_PIN, HIGH); // Aktifkan buzzer
                bot.sendMessage(CHAT_ID, message, "");
                turnonfan(255);

                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
            else
            {
                digitalWrite(BUZZER_PIN, LOW);
                ESP_LOGI("Dapur 2", "Tidak ada kebocoran gas");
            }
            xSemaphoreGive(xBotMutex);
        }
        else
        {
            ESP_LOGE("Mutex Status", "Mutex Failed");
        }

        gasValue1 = 0;
        gasValue2 = 0;

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

boolean updateGasSensorA()
{
    gasValue1 = analogRead(GAS_SENSOR_PIN_1);
    // ESP_LOGI("Update Sensor A", "value: %i", gasValue1);

    if (gasValue1 > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

boolean updateGasSensorB()
{
    gasValue2 = analogRead(GAS_SENSOR_PIN_2);
    // ESP_LOGI("Update Sensor B", "value: %i", gasValue2);

    if (gasValue2 < 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}

void taskTelegram(void *pvParameters)
{
    for (;;)
    {

        ESP_LOGI("Task Telegram", "Running");

        if (millis() > lastTimeBotRan + botRequestDelay)
        {
            if (xSemaphoreTake(xBotMutex, portMAX_DELAY) == pdTRUE)
            {
                int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

                while (numNewMessages)
                {
                    ESP_LOGI("Task Telegram Status", "Pesan didapatkan");
                    handleNewMessages(numNewMessages);
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    numNewMessages = bot.getUpdates(bot.last_message_received + 1);
                }
                xSemaphoreGive(xBotMutex);
                lastTimeBotRan = millis();
            }
            else
            {
                ESP_LOGE("Mutex Status", "Mutex Failed");
            }
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void handleNewMessages(int numNewMessages)
{
    // ESP_LOGI("Telegram", "Jumlah pesan baru %s", String(numNewMessages));

    for (int i = 0; i < numNewMessages; i++)
    {
        ESP_LOGI("Task", "handleNewMessages");
        // Asal request chat id
        String chat_id = String(bot.messages[i].chat_id);

        if (chat_id != CHAT_ID)
        {
            bot.sendMessage(chat_id, "Unauthorized user", "");
            continue;
        }

        String text = bot.messages[i].text;
        ESP_LOGI("Telegram", "isi pesan yang sedang di proses %s", text);

        String from_name = bot.messages[i].from_name;

        if (text == "/start")
        {
            String message = "Selamat Datang, " + from_name + ".\n\n";
            message += "GasGuard Pro (Project Smart Kitchen Industry).\n";
            message += "Sistem deteksi kebocoran Gas LPG untuk melindungi Dapur Industri Anda agar terhindar dari kebakaran!!.\n\n";
            message += "Gunakan perintah berikut agar anda dapat mengontrol sistem GasGuard Pro.\n\n";
            message += "/streaming = untuk melihat situasi keadaan terkini pada dapur anda.\n";
            message += "/turnon_exhaust_fan_pwm100 = untuk mengaktifkan kipas agar membuang kadar gas di dapur dengan kecepatan PWM 100.\n";
            message += "/turnoff_exhaust_fan = untuk menonaktifkan kipas pembuangan.\n";
            message += "/about = untuk mengetahui fitur dan berkenalan lebih jauh tentang sistem ini.\n";
            bot.sendMessage(chat_id, message, "");
            // vTaskDelay(500 / portTICK_PERIOD_MS);
        }

        if (text == "/streaming")
        {
            String message = "IP ADD: " + camURL +"/mjpeg/1";
            bot.sendMessage(chat_id, message, "");
            // vTaskDelay(500 / portTICK_PERIOD_MS);
        }

        if (text == "/turnon_exhaust_fan_pwm100")
        {
            turnonfan(100);
            String message = "Exhaust Fan (PWM 100) telah diaktifkan!.\n";
            bot.sendMessage(chat_id, message, "");
            // vTaskDelay(500 / portTICK_PERIOD_MS);
        }

        if (text == "/turnoff_exhaust_fan")
        {
            turnofffan();
            String message = "Exhaust Fan telah dinonaktifkan!.\n";
            bot.sendMessage(chat_id, message, "");
            // vTaskDelay(500 / portTICK_PERIOD_MS);
        }

        if (text == "/about")
        {
            String message = "Dalam era digital saat ini, ada kebutuhan mendesak akan solusi terpadu yang cerdas untuk memantau kebocoran gas, terutama gas LPG (Liquified Petroleum Gas) di lingkungan dapur industri. Keamanan menjadi prioritas utama, di mana deteksi dini kebocoran gas menjadi krusial untuk menghindari potensi bahaya kebakaran. Penggunaan teknologi Internet of Things (IoT) menjadi semakin relevan dalam hal ini. Dengan menggunakan perangkat seperti ESP-32 dan sensor MQ-2, sistem deteksi ini dapat memberikan pemantauan yang akurat terhadap kebocoran gas. Dukungan dari kecerdasan buatan (AI) melalui OpenCV pada ESP32-CAM juga memungkinkan pemantauan visual yang responsif terhadap deteksi kebakaran.\n\n";
            message += "Sistem 'GasGuard Pro' menyediakan solusi terpadu dan cerdas untuk deteksi kebocoran gas LPG, meningkatkan tingkat keamanan dan memberikan ketenangan pikiran kepada pengguna.\n\n";
            message += "Kami juga mengimplementasikan kontrol aksi PWM pada sistem ini untuk mengatur kecepatan kipas pembuangan (exhaust fan). Jika terdeteksi kebocoran gas, kipas akan berjalan pada kecepatan maksimum. Hal ini memungkinkan pengaliran dan pengeluaran kadar gas di dapur sehingga keamanan lebih terjaga.\n";
            bot.sendMessage(chat_id, message, "");
            // vTaskDelay(500 / portTICK_PERIOD_MS);
        }

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void keepWiFiAlive(void *pvParameters)
{
    for (;;)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            ESP_LOGI("WiFi", "Still connected");
            vTaskDelay(10000 / portTICK_PERIOD_MS);
            continue;
        }

        // Menghubungkan ke jaringan WiFi
        ESP_LOGI("WiFi", "Connecting...");
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        configTime(0, 0, "pool.ntp.org");


        unsigned long startAttemptTime = millis();

        // Terus looping jika tidak terhubung dan belum mencapai WIFI_TIMEOUT
        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS)
        {
        }

        // Kondisi akan terpenuhi ketika tidak dapat terhubung ke WiFi
        if (WiFi.status() != WL_CONNECTED)
        {
            ESP_LOGI("WiFi", "Failed to connect");
            vTaskDelay(20000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI("WiFi", "Connected to %s", WiFi.localIP().toString().c_str());
    }
}

void turnonfan(int pwnSpeed)
{
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, pwnSpeed);
}

void turnofffan()
{
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(ENA, LOW);
}
