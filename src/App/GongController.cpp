#include "App/GongController.h"
#include "System/TimerService.h"

#ifdef __AVR__
#include <avr/io.h>
#endif

namespace App {

// Pin definitions: PD4 = Upperfloor, PD5 = Groundfloor
#ifdef __AVR__
#define GONG_UPPER_PIN    PD4
#define GONG_GROUND_PIN   PD5
#define GONG_DDR          DDRD
#define GONG_PORT         PORTD
#endif

GongController::GongController()
    : default_duration_ms_(kDefaultDurationMs) {
  upperfloor_.enabled = true;
  upperfloor_.active = false;
  upperfloor_.stop_time = 0;

  groundfloor_.enabled = true;
  groundfloor_.active = false;
  groundfloor_.stop_time = 0;
}

void GongController::init() {
#ifdef __AVR__
  // Set PD4 and PD5 as outputs
  GONG_DDR |= (1 << GONG_UPPER_PIN) | (1 << GONG_GROUND_PIN);

  // Set both outputs LOW initially
  GONG_PORT &= ~((1 << GONG_UPPER_PIN) | (1 << GONG_GROUND_PIN));
#endif
}

void GongController::update() {
  uint32_t now = System::TimerService::millis();

  // Check upperfloor gong timeout
  if (upperfloor_.active && now >= upperfloor_.stop_time) {
    set_output(GongId::kUpperfloor, false);
    upperfloor_.active = false;
  }

  // Check groundfloor gong timeout
  if (groundfloor_.active && now >= groundfloor_.stop_time) {
    set_output(GongId::kGroundfloor, false);
    groundfloor_.active = false;
  }
}

void GongController::trigger(GongId gong, bool force, uint16_t duration_ms) {
  if (duration_ms == 0) {
    duration_ms = default_duration_ms_;
  }

  // Clamp duration
  if (duration_ms < kMinDurationMs) duration_ms = kMinDurationMs;
  if (duration_ms > kMaxDurationMs) duration_ms = kMaxDurationMs;

  uint32_t stop_time = System::TimerService::millis() + duration_ms;

  switch (gong) {
    case GongId::kUpperfloor:
      if (force || upperfloor_.enabled) {
        set_output(GongId::kUpperfloor, true);
        upperfloor_.active = true;
        upperfloor_.stop_time = stop_time;
      }
      break;

    case GongId::kGroundfloor:
      if (force || groundfloor_.enabled) {
        set_output(GongId::kGroundfloor, true);
        groundfloor_.active = true;
        groundfloor_.stop_time = stop_time;
      }
      break;

    case GongId::kBoth:
      if (force || upperfloor_.enabled) {
        set_output(GongId::kUpperfloor, true);
        upperfloor_.active = true;
        upperfloor_.stop_time = stop_time;
      }
      if (force || groundfloor_.enabled) {
        set_output(GongId::kGroundfloor, true);
        groundfloor_.active = true;
        groundfloor_.stop_time = stop_time;
      }
      break;
  }
}

void GongController::set_enabled(GongId gong, bool enabled) {
  switch (gong) {
    case GongId::kUpperfloor:
      upperfloor_.enabled = enabled;
      break;
    case GongId::kGroundfloor:
      groundfloor_.enabled = enabled;
      break;
    case GongId::kBoth:
      upperfloor_.enabled = enabled;
      groundfloor_.enabled = enabled;
      break;
  }
}

bool GongController::is_enabled(GongId gong) const {
  switch (gong) {
    case GongId::kUpperfloor:
      return upperfloor_.enabled;
    case GongId::kGroundfloor:
      return groundfloor_.enabled;
    case GongId::kBoth:
      return upperfloor_.enabled && groundfloor_.enabled;
  }
  return false;
}

bool GongController::is_active(GongId gong) const {
  switch (gong) {
    case GongId::kUpperfloor:
      return upperfloor_.active;
    case GongId::kGroundfloor:
      return groundfloor_.active;
    case GongId::kBoth:
      return upperfloor_.active || groundfloor_.active;
  }
  return false;
}

void GongController::set_duration(uint16_t duration_ms) {
  if (duration_ms < kMinDurationMs) duration_ms = kMinDurationMs;
  if (duration_ms > kMaxDurationMs) duration_ms = kMaxDurationMs;
  default_duration_ms_ = duration_ms;
}

uint16_t GongController::get_duration() const {
  return default_duration_ms_;
}

void GongController::trigger_enabled_gongs() {
  // With the new trigger signature, just trigger both - the enabled check is inside trigger()
  trigger(GongId::kBoth, false);
}

void GongController::set_output(GongId gong, bool state) {
#ifdef __AVR__
  switch (gong) {
    case GongId::kUpperfloor:
      if (state) {
        GONG_PORT |= (1 << GONG_UPPER_PIN);
      } else {
        GONG_PORT &= ~(1 << GONG_UPPER_PIN);
      }
      break;

    case GongId::kGroundfloor:
      if (state) {
        GONG_PORT |= (1 << GONG_GROUND_PIN);
      } else {
        GONG_PORT &= ~(1 << GONG_GROUND_PIN);
      }
      break;

    case GongId::kBoth:
      if (state) {
        GONG_PORT |= (1 << GONG_UPPER_PIN) | (1 << GONG_GROUND_PIN);
      } else {
        GONG_PORT &= ~((1 << GONG_UPPER_PIN) | (1 << GONG_GROUND_PIN));
      }
      break;
  }
#else
  (void)gong;
  (void)state;
#endif
}

}  // namespace App
