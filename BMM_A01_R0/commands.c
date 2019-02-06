#include "driver_init.h"
#include "commands.h"
#include "subbus.h"

static void update_status(uint16_t *status, uint8_t pin, uint16_t bit) {
  if (gpio_get_pin_level(pin)) {
    *status |= bit;
  } else {
    *status &= ~bit;
  }
}

static void cmd_poll(void) {
  uint16_t cmd;
  uint16_t status;
  if (subbus_cache_iswritten(&sb_cmd, CMD_BASE_ADDR, &cmd)) {
    switch (cmd) {
      case 0: gpio_set_pin_level(STATUS_LED, false); break;
      case 1: gpio_set_pin_level(STATUS_LED, true); break;
      case 2: gpio_set_pin_level(FAULT_LED, false); break;
      case 3: gpio_set_pin_level(FAULT_LED, true); break;
      case 4: gpio_set_pin_level(SHDN_N, false); break;
      case 5: gpio_set_pin_level(SHDN_N, true); break;
      default:
        break;
    }
  }
  status = 0;
  update_status(&status, STATUS_LED, 0x01);
  update_status(&status, FAULT_LED, 0x02);
  update_status(&status, SHDN_N, 0x04);
  subbus_cache_update(&sb_cmd, CMD_BASE_ADDR, status);
}

static void cmd_reset(void) {
  if (!sb_cmd.initialized) {
    sb_cmd.initialized = true;
  }
}

/**
 * This file should include a memory map. The current one is In Evernote.
 * 0x10-0x13 R: ADC Flow values
 * 0x14-0x17 RW: DAC Flow Setpoints
 * 0x18 R: CmdStatus W: Command
 * 0x19 R: ADC_U2_T
 * 0x1A R: ADC_U3_T
 */
static subbus_cache_word_t cmd_cache[CMD_HIGH_ADDR-CMD_BASE_ADDR+1] = {
  { 0, 0, true,  false, true, false, false } // Offset 0: R: ADC Flow 0
};

subbus_driver_t sb_cmd = {
  CMD_BASE_ADDR, CMD_HIGH_ADDR, // address range
  cmd_cache,
  cmd_reset,
  cmd_poll,
  0, // dynamic driver
  false
};
