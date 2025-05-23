#include "Oximeter.h"

Oximeter::Oximeter(uint8_t sdaPin, uint8_t sclPin)
  : spo2(0), heartRate(0)
{
  Wire.begin(sdaPin, sclPin);
  Wire.setClock(400000);  // I2C’yi 400kHz’e çıkar
  // Kuyruk: MAX_SAMPLES * 2 derinlik, her eleman Sample boyutunda
  sampleQueue = xQueueCreate(MAX_SAMPLES * 2, sizeof(Sample));
}

bool Oximeter::begin() {
  if (!sensor.begin(Wire, I2C_SPEED_FAST)) return false;
  sensor.setup();                        // varsayılan ayarlar
  sensor.setPulseAmplitudeRed(0x0F);     // LED gücünü maksimuma çek
  sensor.setPulseAmplitudeIR(0x0F);
  return true;
}

void Oximeter::startTasks() {
  // Core 0’da okumayı, Core 1’de işlemeyi çalıştır
  xTaskCreatePinnedToCore(samplingTask, "OxiSampler", 4096, this, 1, nullptr, 0);
  xTaskCreatePinnedToCore(processingTask, "OxiProcessor", 8192, this, 1, nullptr, 1);
}

int Oximeter::getSpO2() const      { return spo2; }
int Oximeter::getHeartRate() const { return heartRate; }

void Oximeter::samplingTask(void *param) {
  auto *ox = static_cast<Oximeter*>(param);
  Sample s;
  for (;;) {
    // Kuyruğa at, 0 blok süresi
    s.red = ox->sensor.getRed();
    s.ir  = ox->sensor.getIR();
    xQueueSend(ox->sampleQueue, &s, 0);
    // hemen bir sonraki örneğe geç
  }
}

void Oximeter::processingTask(void *param) {
  auto *ox = static_cast<Oximeter*>(param);
  Sample buf[MAX_SAMPLES];
  for (;;) {
    // Kuyruktan MAX_SAMPLES tane örnek al
    for (int i = 0; i < MAX_SAMPLES; ++i) {
      xQueueReceive(ox->sampleQueue, &buf[i], portMAX_DELAY);
    }
    // Hesaplamayı yap
    ox->processBuffer(buf, MAX_SAMPLES);
    // 1 saniye bekle, sonra tekrar
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void Oximeter::processBuffer(const Sample *buf, size_t len) {
  // Geçiçi raw diziler
  static uint32_t irB[MAX_SAMPLES], redB[MAX_SAMPLES];
  for (int i = 0; i < len; ++i) {
    irB[i]  = buf[i].ir;
    redB[i] = buf[i].red;
  }

  int8_t spo2_valid, hr_valid;
  int hr;
  maxim_heart_rate_and_oxygen_saturation(
    irB, len, redB,
    &spo2, &spo2_valid,
    &hr,  &hr_valid
  );

  spo2      = spo2_valid ? spo2 : 0;
  heartRate = hr_valid   ? hr   : 0;
}
