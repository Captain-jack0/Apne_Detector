#ifndef OXIMETER_H
#define OXIMETER_H

#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/// Basit veri yapısı: bir örnek için Red ve IR değeri
struct Sample {
  uint32_t red;
  uint32_t ir;
};

class Oximeter {
public:
  Oximeter(uint8_t sdaPin = 21, uint8_t sclPin = 22);

  /// I2C ve sensörü başlatır
  bool begin();

  /// Sampling ve processing task’lerini ayağa kaldırır
  void startTasks();

  /// En son işlenmiş değerler
  int getSpO2() const;
  int getHeartRate() const;

  /// Task’ler arasında örnek paylaşımı için kuyruk
  QueueHandle_t sampleQueue;

private:
  static const int MAX_SAMPLES = 100;  // azaltılmış örnek sayısı

  MAX30105 sensor;
  int spo2, heartRate;

  /// Internal: buffer’ı işleyip spo2/hr hesaplar
  void processBuffer(const Sample *buf, size_t len);

  /// Task fonksiyonları
  static void samplingTask(void *param);
  static void processingTask(void *param);
};

#endif // OXIMETER_H
