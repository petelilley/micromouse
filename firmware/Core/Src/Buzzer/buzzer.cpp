#include "Buzzer/buzzer.hpp"

#include "Buzzer/notes.hpp"

#include "stm32wbxx_hal.h"
#include <span>

//
// External variables.
//

extern TIM_HandleTypeDef htim16; // main.c

//
// Types.
//

struct SongHandle {
  std::span<const Note> notes;
  uint16_t ticks_per_note;
  bool pause_between_notes = true;
};

//
// Startup tone.
//

static constexpr uint16_t SONG_STARTUP_TEMPO = 250;

static constexpr Note SONG_STARTUP_NOTES[] = {
    NOTE_D_5,
    NOTE_D_6,
};

//
// Home Depot theme song.
//

static constexpr uint16_t SONG_HOME_DEPOT_TEMPO = 250;

// clang-format off
static constexpr Note SONG_HOME_DEPOT_NOTES[] = {
    // Intro
    NOTE_A_4, NOTE_A_4, NOTE_D_5, NOTE_A_4, REST, NOTE_A_4, REST, NOTE_A_4, NOTE_C_5, NOTE_A_4, REST, NOTE_A_4, REST, NOTE_A_4, NOTE_G_4, NOTE_A_4,
    NOTE_A_4, NOTE_A_4, NOTE_D_5, NOTE_A_4, REST, NOTE_A_4, REST, NOTE_A_4, NOTE_C_5, NOTE_A_4, REST, NOTE_A_4, REST, NOTE_A_4, NOTE_G_4, NOTE_A_4,
    NOTE_A_4, NOTE_A_4, NOTE_D_5, NOTE_A_4, REST, NOTE_A_4, REST, NOTE_A_4, NOTE_C_5, NOTE_A_4, REST, NOTE_A_4, REST, NOTE_A_4, NOTE_G_4, NOTE_A_4,

    // Transition
    REST, NOTE_A_4, NOTE_D_5, NOTE_A_4, NOTE_C_5, NOTE_D_6, REST,

    // Loop
    NOTE_A_4, NOTE_D_5, NOTE_A_4, NOTE_C_5, NOTE_A_4, NOTE_G_4, NOTE_D_6, REST,
    NOTE_A_4, NOTE_D_5, NOTE_A_4, NOTE_C_5, NOTE_A_4, NOTE_G_4, NOTE_D_6, REST,
    NOTE_A_4, NOTE_D_5, NOTE_A_4, NOTE_C_5, NOTE_A_4, NOTE_G_4, NOTE_D_6, REST,
    NOTE_A_4, NOTE_D_5, NOTE_A_4, NOTE_C_5, NOTE_A_4, NOTE_G_4, NOTE_D_6, REST,
    NOTE_A_4, NOTE_D_5, NOTE_A_4, NOTE_C_5, NOTE_A_4, NOTE_G_4, NOTE_D_6, REST,
    NOTE_A_4, NOTE_D_5, NOTE_A_4, NOTE_C_5, NOTE_A_4, NOTE_G_4, NOTE_D_6, REST,
    NOTE_A_4, NOTE_D_5, NOTE_A_4, NOTE_C_5, NOTE_A_4, NOTE_G_4, NOTE_D_6, REST,

    // End
    NOTE_A_4, NOTE_D_5, NOTE_A_4, NOTE_A_4,
};
// clang-format on

//
// Nokia ringtone.
//

static constexpr uint16_t SONG_NOKIA_TEMPO = 180;

static constexpr Note SONG_NOKIA_NOTES[] = {
    NOTE_E_6,  NOTE_D_6, NOTE_FS_5, NOTE_FS_5, NOTE_GS_5, NOTE_GS_5,
    NOTE_CS_6, NOTE_B_5, NOTE_D_5,  NOTE_D_5,  NOTE_E_5,  NOTE_E_5,
    NOTE_B_5,  NOTE_A_5, NOTE_CS_5, NOTE_CS_5, NOTE_E_5,  NOTE_E_5,
    NOTE_A_5,  NOTE_A_5, NOTE_A_5,  NOTE_A_5,  REST, REST,
    NOTE_E_6,  NOTE_D_6, NOTE_FS_5, NOTE_FS_5, NOTE_GS_5, NOTE_GS_5,
    NOTE_CS_6, NOTE_B_5, NOTE_D_5,  NOTE_D_5,  NOTE_E_5,  NOTE_E_5,
    NOTE_B_5,  NOTE_A_5, NOTE_CS_5, NOTE_CS_5, NOTE_E_5,  NOTE_E_5,
    NOTE_A_5,  NOTE_A_5, NOTE_A_5,  NOTE_A_5,  REST,
};

//
// All songs.
//

static constexpr SongHandle SONGS[] = {
    // NONE
    {},

    // STARTUP
    {SONG_STARTUP_NOTES, SONG_STARTUP_TEMPO / ROBOT_UPDATE_PERIOD_MS},

    // HOME_DEPOT
    {SONG_HOME_DEPOT_NOTES, SONG_HOME_DEPOT_TEMPO / ROBOT_UPDATE_PERIOD_MS},

    // NOKIA
    {SONG_NOKIA_NOTES, SONG_NOKIA_TEMPO / ROBOT_UPDATE_PERIOD_MS, false},
};

//
// Buzzer functions.
//

void Buzzer::init() { play_song(Buzzer::Song::STARTUP); }

void Buzzer::process() {
  if (!m_song_handle) return;

  if (m_note_ticks++ == 0) {
    const Note note = m_song_handle->notes[m_note_index];
    set_note(note);
  }

  // End of a note.
  if (m_note_ticks == m_song_handle->ticks_per_note) {
    m_note_ticks = 0;

    if (m_song_handle->pause_between_notes) {
      // Add a small rest to separate notes.
      set_note(0);
    }

    // Check if this is the last note in the song.
    if (++m_note_index == m_song_handle->notes.size()) {
      // Stop playing.
      m_song_handle = nullptr;
    }
  }
}

void Buzzer::play_song(Song song) {
  m_song_handle = &SONGS[static_cast<uint8_t>(song)];
  m_note_index  = 0;
  m_note_ticks  = 0;
}

void Buzzer::quiet() {
  m_song_handle = nullptr;
  set_note(0);
}

void Buzzer::set_note(uint32_t counter_period) {
  htim16.Instance->ARR = counter_period;

  // 50% duty cycle for a square wave.
  htim16.Instance->CCR1 = counter_period / 2;
}
