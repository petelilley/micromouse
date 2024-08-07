#pragma once

#include "Basic/singleton.hpp"

#include "Maze/coordinate.hpp"
#include "Maze/maze.hpp"

#include "Basic/error_manager.hpp"
#include "Buzzer/buzzer.hpp"
#include "Drive/drive.hpp"
#include "Drive/drive_controller.hpp"
#include "Navigation/navigator.hpp"
#include "Vision/vision.hpp"

#include <array>

class Robot : public Singleton<Robot> {
  Maze m_maze;

  Buzzer m_buzzer;
  Vision m_vision;
  Drive m_drive;

  DriveController m_drive_controller {m_drive};
  Navigator m_navigator {m_drive_controller, m_maze};

  Maze::StartLocation m_start_location = Maze::StartLocation::WEST_OF_GOAL;

  const std::array<Subsystem*, 6> m_subsystems = {&ErrorManager::get(),
                                                  &m_navigator,
                                                  &m_drive_controller,
                                                  &m_drive,
                                                  &m_buzzer,
                                                  &m_vision};

  bool m_ble_connected = false;

private:
  std::array<maze::CoordinateSpan, 4> get_search_targets() {
    return {Maze::GOAL_ENDPOINTS, Maze::outside_start_span(m_start_location),
            Maze::GOAL_ENDPOINTS, Maze::start_span(m_start_location)};
  }

  std::array<maze::CoordinateSpan, 4> get_solve_targets() {
    return {Maze::GOAL_ENDPOINTS, Maze::start_span(m_start_location)};
  }

public:
  Buzzer& buzzer() { return m_buzzer; }
  Vision& vision() { return m_vision; }
  Drive& drive() { return m_drive; }

  //
  // General robot control stuff.
  //
private:
  friend void ::Robot_Init(void);
  friend void ::Robot_Update(void);
  friend void ::Robot_OnConnect(void);
  friend void ::Robot_DashboardAppReady(void);
  friend void ::Robot_OnDisconnect(void);
  friend void ::Robot_SendFeedback(void);

  void init();
  void update();
  void on_connect();
  void dashboard_app_ready();
  void on_disconnect();
  void send_feedback();

  //
  // Task management stuff.
  //
public:
  enum class Task : uint8_t {
    NONE            = 0,
    MAZE_SEARCH     = 1,
    MAZE_SLOW_SOLVE = 2,
    MAZE_FAST_SOLVE = 3,

    TEST_DRIVE_STRAIGHT   = 4,
    TEST_DRIVE_LEFT_TURN  = 5,
    TEST_DRIVE_RIGHT_TURN = 6,

    TEST_GYRO = 7,

    ARMED,
  };

private:
  Task m_task = Task::NONE;

  Task m_next_task    = Task::NONE;
  bool m_is_next_task = false;

  Task m_armed_task = Task::NONE;

  bool m_search_done = false;

  enum class SearchStage : uint8_t {
    START_TO_GOAL         = 0,
    GOAL_TO_OUTSIDE_START = 1,
    OUTSIDE_START_TO_GOAL = 2,
    GOAL_TO_START         = 3,
  } m_search_stage = SearchStage::START_TO_GOAL;

  enum class SolveStage : uint8_t {
    START_TO_GOAL = 0,
    GOAL_TO_START = 1,
  } m_solve_stage = SolveStage::START_TO_GOAL;

public:
  Task current_task() const {
    if (m_is_next_task) return m_next_task;
    return m_task;
  }

private:
  friend void ::HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

  void handle_button_1();
  void handle_button_2();

private:
  friend void ::RobotControl_RunTask(uint8_t task, uint8_t start_location);
  friend void ::RobotControl_ResetMaze(void);

  void arm_task(Task task);
  void run_task(Task task);

  // Signals that the current task is done, and performs any necessary cleanup.
  void end_task();

  // Erases all knowledge of the maze.
  void reset_maze();

  void set_start_location(Maze::StartLocation start_location) {
    m_start_location = start_location;
  }

private:
  void start_next_task();

  void start_task_maze_search();
  void start_task_maze_solve(bool fast);
  void start_task_test_gyro();

  void process_current_task();

  void process_task_maze_search();
  void process_task_maze_solve(bool fast);

  void process_armed();

private:
  void send_current_task();
};
