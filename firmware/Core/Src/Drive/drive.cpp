#include "Drive/drive.hpp"

#include <cmath>

static constexpr int8_t PWM_PERIOD = 10; // The resolution of the PWM.

void Drive::process() {}

void Drive::set_speed(float left_float, float right_float) {
  const int8_t left(left_float * PWM_PERIOD);
  const int8_t right(right_float * PWM_PERIOD);

  const int8_t abs_left  = std::abs(left);
  const int8_t abs_right = std::abs(right);

  const GPIO_PinState left_dir =
      (abs_left / left) > 0 ? GPIO_PIN_RESET : GPIO_PIN_SET;
  const GPIO_PinState right_dir =
      (abs_right / right) > 0 ? GPIO_PIN_RESET : GPIO_PIN_SET;

  set_speed_dir(abs_left, left_dir, abs_right, right_dir);
}

void Drive::set_speed_dir(uint8_t left, GPIO_PinState left_dir, uint8_t right,
                          GPIO_PinState right_dir) {
  HAL_GPIO_WritePin(MOTOR_LEFT_DIR_GPIO_Port, MOTOR_LEFT_DIR_Pin, left_dir);
  HAL_GPIO_WritePin(MOTOR_RIGHT_DIR_GPIO_Port, MOTOR_RIGHT_DIR_Pin, right_dir);

  pwm_pulse_left  = left;
  pwm_pulse_right = right;
}

void Drive::pwm_update() {
  if (pwm_counter < pwm_pulse_right) {
    MOTOR_RIGHT_GPIO_Port->BSRR = uint32_t(MOTOR_RIGHT_Pin); // Set
  } else {
    MOTOR_RIGHT_GPIO_Port->BRR = uint32_t(MOTOR_RIGHT_Pin); // Reset
  }

  if (pwm_counter < pwm_pulse_left) {
    MOTOR_LEFT_GPIO_Port->BSRR = uint32_t(MOTOR_LEFT_Pin); // Set
  } else {
    MOTOR_LEFT_GPIO_Port->BRR = uint32_t(MOTOR_LEFT_Pin); // Reset
  }

  pwm_counter++;
  pwm_counter %= PWM_PERIOD;
}