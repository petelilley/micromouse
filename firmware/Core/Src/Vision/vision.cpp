#include "Vision/vision.hpp"

#include "custom_stm.h"
#include <cmath>

//
// External variables.
//

extern ADC_HandleTypeDef hadc1; // main.c

//
// Constants.
//

static constexpr GPIO_TypeDef* EMIT_PORTS[] = {
    IR_FAR_RIGHT_EMIT_GPIO_Port,
    IR_MID_RIGHT_EMIT_GPIO_Port,
    IR_MID_LEFT_EMIT_GPIO_Port,
    IR_FAR_LEFT_EMIT_GPIO_Port,
};

static constexpr uint16_t EMIT_PINS[] = {
    IR_FAR_RIGHT_EMIT_Pin,
    IR_MID_RIGHT_EMIT_Pin,
    IR_MID_LEFT_EMIT_Pin,
    IR_FAR_LEFT_EMIT_Pin,
};

//
// Vision functions.
//

void Vision::process() {
  if (!m_enabled) {
    if (m_state != State::IDLE) {
      // Turn off the emitter.
      set_emitter(m_sensor, GPIO_PIN_RESET);
      m_state = State::IDLE;
    }
    return;
  }

  switch (m_state) {
  case State::READING:
    if (!m_adc_ready) return;
    m_adc_ready = false;
    m_state     = State::IDLE;

    // Turn off the emitter.
    set_emitter(m_sensor, GPIO_PIN_RESET);

    {
      const uint8_t& reading = m_raw_readings[m_sensor];
      m_readings[m_sensor]   = reading;
      m_distances[m_sensor]  = calculate_distance_mm(float(reading));
    }

    // Next sensor.
    m_sensor = static_cast<Sensor>((m_sensor + 1) % 4);
    break;

  case State::IDLE:
    // Turn on the emitter.
    set_emitter(m_sensor, GPIO_PIN_SET);

    m_state = State::WAITING; // Wait one tick for the emitter to turn on.
    break;

  case State::WAITING:
    // Start DMA read.
    HAL_ADC_Start_DMA(&hadc1, reinterpret_cast<uint32_t*>(&m_raw_readings), 4);
    m_state = State::READING;
    break;
  }
}

void Vision::set_emitter(Sensor sensor, GPIO_PinState state) {
  HAL_GPIO_WritePin(EMIT_PORTS[sensor], EMIT_PINS[sensor], state);
}

float Vision::calculate_distance_mm(const float& R) {
  if (R < 1.5f) return std::numeric_limits<float>::infinity();

  const float known_distance_mm = 115.f;
  const float known_intensity   = 5.f;

  const float K = known_intensity * 4 * (known_distance_mm * known_distance_mm);

  // The light intensity emitted gets weaker as the distance increases (by the
  // inverse square law). The distance is actually double, since it needs to
  // travel away from the robot and back after being reflected.
  //
  // R = 1 / (2d)^2
  //
  // To solve for distance, use a known intensity and distance to solve for K.
  //
  // K = R * (2d)^2
  //
  // Solve for d.
  //
  // d = sqrt(K / (4 * R))
  const float d = std::sqrt(K / (4.f * R));

  return d;
}

void Vision::read_complete_handler() { m_adc_ready = true; }

void Vision::send_feedback() {
  if (!m_enabled) return;

}

#include "Basic/robot.hpp"

// ADC conversion complete callback.
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
  assert_param(hadc->Instance == ADC1);
  UNUSED(hadc);

  Robot::get().vision().read_complete_handler();
}
