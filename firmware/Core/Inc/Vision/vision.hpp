#pragma once

#include "Basic/subsystem.hpp"

#include <cstdint>

class Vision : public Subsystem {
  bool m_enabled = false;

public:
  enum Sensor : uint8_t {
    FAR_RIGHT = 0, // Channel 7
    MID_RIGHT,     // Channel 8
    MID_LEFT,      // Channel 9
    FAR_LEFT,      // Channel 10
  };

private:
  uint16_t m_raw_readings[4]; // From ADC DMA.
  float m_readings[4];
  float m_distances[4];
  float m_adjusted_distances[4];

  Sensor m_sensor = Sensor::FAR_RIGHT;

  enum class State {
    IDLE,
    WAITING,
    READING,
  };

  State m_state = State::IDLE;

  volatile bool m_adc_ready = false;

public:
  void process() override;
  void send_feedback() override;

  void set_enabled(bool enabled) { m_enabled = enabled; };
  bool enabled() const { return m_enabled; };

  uint8_t get_raw_reading(Sensor sensor) const { return m_readings[sensor]; };

  // Distance to object directly in front of respective sensor.
  float get_distance_mm(Sensor sensor) const { return m_distances[sensor]; };

  // Distance from the sensor to object (accounting for the sensor's angle).
  float get_adjusted_distance_mm(Sensor sensor) const {
    return m_adjusted_distances[sensor];
  }

private:
  void set_emitter(Sensor sensor, GPIO_PinState state);

  // Compensate for the inverse square law and calculate the distance to an
  // object based on the intensity of light received back from the
  // phototransistor.
  //
  // Intensity should range between 0 and 1 (1 being the maximum intensity
  // capable of being read).
  static float calculate_distance_mm(const float& intensity_reading);

  // Adjusts a distance reading to account for the angle of the sensor.
  static float adjust_distance_mm(const float& distance, Sensor sensor);

private:
  friend void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef*);

  void read_complete_handler();
};
