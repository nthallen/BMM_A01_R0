/** @file i2c.c */
#include "driver_init.h"
#include <utils.h>
#include <hal_init.h>
#include <hal_i2c_m_async.h>
#include "atmel_start_pins.h"
#include "i2c.h"
#include "subbus.h"

static bool i2c_enabled = I2C_ENABLE_DEFAULT;
static struct io_descriptor *I2C_io;
static volatile bool I2C_txfr_complete = true;
static volatile bool I2C_error_seen = false;
static volatile int32_t I2C_error = I2C_OK;
static int32_t pm_i2c_error = I2C_OK;
static volatile uint8_t pm_ov_status = 0;
static uint8_t pm_ibuf[6];
#define PM_SLAVE_ADDR 0x67
#define PM_OVERFLOW 1
#define PM_UNDERFLOW 2

enum pm_state_t {pm_init, pm_init_tx};
static enum pm_state_t pm_state = pm_init;

/**
 * These addresses belong to the I2C module
 * 0x20 R:  I2C_Status
 * 0x21 R:  PwrMon_I
 * 0x22 R:  PwrMon_V
 * 0x23 R:  PwrMon_V2
 * 0x24 R:  PwrMon_N
 * 0x25 R:  T1
 * 0x26 R:  T2
 */
static subbus_cache_word_t i2c_cache[I2C_HIGH_ADDR-I2C_BASE_ADDR+1] = {
  { 0, 0, true,  false,  false, false, false }, // Offset 0: R: I2C Status
  { 0, 0, true,  false, false, false, false },  // Offset 1: R: PwrMon_I
  { 0, 0, true,  false, false, false, false },  // Offset 2: R: PwrMon_V
  { 0, 0, true,  false, false, false, false },  // Offset 3: R: PwrMon_V2
  { 0, 0, true,  false, false, false, false },  // Offset 4: R: PwrMon_N
  { 0, 0, true,  false, false, false, false },  // Offset 5: R: PwrMon_Status
  { 0, 0, true,  false,  true, false, false },  // Offset 6: R: T1
  { 0, 0, true,  false, false, false, false },  // Offset 7: R: T2
  { 0, 0, true,  false, false, false, false }   // Offset 8: R: ADS_N
};

static void  pm_record_i2c_error(enum pm_state_t pm_state, int32_t I2C_error) {
  uint16_t word = ((pm_state & 0x7) << 4) | (I2C_error & 0xF);
  i2c_cache[5].cache = (i2c_cache[5].cache & 0xFF00) | word;
}

static void pm_record_ov_status(uint8_t ovs) {
  i2c_cache[5].cache = (i2c_cache[5].cache & 0xFCFF) | ((ovs & 3) << 8);
}

/**
 * @return true if the bus is free and available for another device
 */
static bool pm_poll(void) {
  static int n_readings = 0;
  // static int64_t sum = 0;

  if (i2c_cache[1].was_read && i2c_cache[2].was_read && i2c_cache[3].was_read && i2c_cache[4].was_read) {
    n_readings = 0;
    i2c_cache[1].was_read = i2c_cache[2].was_read = i2c_cache[3].was_read = i2c_cache[4].was_read = false;
  }
  if (i2c_cache[5].was_read) {
    pm_ov_status = 0;
    pm_record_ov_status(pm_ov_status);
    I2C_error = I2C_OK;
    i2c_cache[5].was_read = false;
    pm_record_i2c_error(pm_state, I2C_OK);
  }

  switch (pm_state) {
    case pm_init:
      I2C_txfr_complete = false;
      i2c_m_async_set_slaveaddr(&I2C, PM_SLAVE_ADDR, I2C_M_SEVEN);
      io_read(I2C_io, pm_ibuf, 6);
      pm_state = pm_init_tx;
      return false;
    case pm_init_tx:
      if (I2C_error_seen) {
        I2C_error_seen = false;
        pm_i2c_error = I2C_error;
        pm_state = pm_init;
      } else {
        i2c_cache[1].cache = (pm_ibuf[0]<<8) | pm_ibuf[1];
        i2c_cache[2].cache = (pm_ibuf[2]<<8) | pm_ibuf[3];
        i2c_cache[3].cache = (pm_ibuf[4]<<8) | pm_ibuf[5];
        i2c_cache[4].cache = ++n_readings;
        pm_state = pm_init;
      }
      return true;
    default:
      assert(false, __FILE__, __LINE__);
  }
  return true; // will never actually get here
}

enum ads_state_t {ads_t1_init, ads_t1_init_tx, ads_t1_read_cfg,
                  ads_t1_read_cfg_tx, ads_t1_reg0, ads_t1_read_adc,
                  ads_t1_read_adc_tx,
                  ads_t2_init, ads_t2_init_tx, ads_t2_read_cfg,
                  ads_t2_read_cfg_tx, ads_t2_reg0, ads_t2_read_adc,
                  ads_t2_read_adc_tx};
static enum ads_state_t ads_state = ads_t1_init;
static uint16_t ads_n_reads;
static uint8_t ads_t1_cmd[4] = { 0x01, 0x83, 0x00, 0x03 };
static uint8_t ads_t2_cmd[4] = { 0x01, 0xB3, 0x00, 0x03 };
static uint8_t ads_r0_prep[1] = { 0x00 };
static uint8_t ads_ibuf[2];
#define ADS_SLAVE_ADDR 0x48

/**
 * @return true if the bus is free and available for another device
 */
static bool ads1115_poll(void) {
  switch (ads_state) {
    case ads_t1_init:
      ads_n_reads = 0;
      I2C_txfr_complete = false;
      i2c_m_async_set_slaveaddr(&I2C, ADS_SLAVE_ADDR, I2C_M_SEVEN);
      io_write(I2C_io, ads_t1_cmd, 4);
      ads_state = ads_t1_init_tx;
      return false;
    case ads_t1_init_tx:
      ads_state = ads_t1_read_cfg;
      return true;
    case ads_t1_read_cfg:
      I2C_txfr_complete = false;
      i2c_m_async_set_slaveaddr(&I2C, ADS_SLAVE_ADDR, I2C_M_SEVEN);
      io_read(I2C_io, ads_ibuf, 2);
      ads_state = ads_t1_read_cfg_tx;
      return false;
    case ads_t1_read_cfg_tx:
      if (ads_ibuf[0] & 0x80) {
        ads_state = ads_t1_reg0;
      } else {
        ++ads_n_reads;
        ads_state = ads_t1_read_cfg;
      }
      return true;
    case ads_t1_reg0:
      I2C_txfr_complete = false;
      i2c_m_async_set_slaveaddr(&I2C, ADS_SLAVE_ADDR, I2C_M_SEVEN);
      io_write(I2C_io, ads_r0_prep, 1);
      ads_state = ads_t1_read_adc;
      return false;
    case ads_t1_read_adc:
      I2C_txfr_complete = false;
      i2c_m_async_set_slaveaddr(&I2C, ADS_SLAVE_ADDR, I2C_M_SEVEN);
      io_read(I2C_io, ads_ibuf, 2);
      ads_state = ads_t1_read_adc_tx;
      return false;
    case ads_t1_read_adc_tx:
      i2c_cache[6].cache = (ads_ibuf[0] << 8) | ads_ibuf[1];
      i2c_cache[8].cache = ads_n_reads;
      ads_state = ads_t2_init;
      return true;
    case ads_t2_init:
      ads_n_reads = 0;
      I2C_txfr_complete = false;
      i2c_m_async_set_slaveaddr(&I2C, ADS_SLAVE_ADDR, I2C_M_SEVEN);
      io_write(I2C_io, ads_t2_cmd, 4);
      ads_state = ads_t2_init_tx;
      return false;
    case ads_t2_init_tx:
      ads_state = ads_t2_read_cfg;
      return true;
    case ads_t2_read_cfg:
      I2C_txfr_complete = false;
      i2c_m_async_set_slaveaddr(&I2C, ADS_SLAVE_ADDR, I2C_M_SEVEN);
      io_read(I2C_io, ads_ibuf, 2);
      ads_state = ads_t2_read_cfg_tx;
      return false;
    case ads_t2_read_cfg_tx:
      if (ads_ibuf[0] & 0x80) {
        ads_state = ads_t2_reg0;
      } else {
        ++ads_n_reads;
        ads_state = ads_t2_read_cfg;
      }
      return true;
    case ads_t2_reg0:
      I2C_txfr_complete = false;
      i2c_m_async_set_slaveaddr(&I2C, ADS_SLAVE_ADDR, I2C_M_SEVEN);
      io_write(I2C_io, ads_r0_prep, 1);
      ads_state = ads_t2_read_adc;
      return false;
    case ads_t2_read_adc:
      I2C_txfr_complete = false;
      i2c_m_async_set_slaveaddr(&I2C, ADS_SLAVE_ADDR, I2C_M_SEVEN);
      io_read(I2C_io, ads_ibuf, 2);
      ads_state = ads_t2_read_adc_tx;
      return false;
    case ads_t2_read_adc_tx:
      i2c_cache[7].cache = (ads_ibuf[0] << 8) | ads_ibuf[1];
      i2c_cache[8].cache = ads_n_reads;
      ads_state = ads_t1_init;
      return true;
    default:
      assert(false, __FILE__, __LINE__);
  }
  return true;
}

void i2c_enable(bool value) {
  i2c_enabled = value;
}

#define I2C_INTFLAG_ERROR (1<<7)

static void I2C_async_error(struct i2c_m_async_desc *const i2c, int32_t error) {
  I2C_txfr_complete = true;
  I2C_error_seen = true;
  I2C_error = error;
  if (error == I2C_ERR_BUS) {
    hri_sercomi2cm_write_STATUS_reg(I2C.device.hw, SERCOM_I2CM_STATUS_BUSERR);
    hri_sercomi2cm_clear_INTFLAG_reg(I2C.device.hw, I2C_INTFLAG_ERROR);
  }
}

static void I2C_txfr_completed(struct i2c_m_async_desc *const i2c) {
  I2C_txfr_complete = true;
}

static void i2c_reset() {
  if (!sb_i2c.initialized) {
    // I2C_init(); // Called from driver_init
    i2c_m_async_get_io_descriptor(&I2C, &I2C_io);
    i2c_m_async_enable(&I2C);
    i2c_m_async_register_callback(&I2C, I2C_M_ASYNC_ERROR, (FUNC_PTR)I2C_async_error);
    i2c_m_async_register_callback(&I2C, I2C_M_ASYNC_TX_COMPLETE, (FUNC_PTR)I2C_txfr_completed);
    i2c_m_async_register_callback(&I2C, I2C_M_ASYNC_RX_COMPLETE, (FUNC_PTR)I2C_txfr_completed);

    sb_i2c.initialized = true;
  }
}

// static void i2c_may_use() {
  // i2c_m_async_set_slaveaddr(&I2C, 0x12, I2C_M_SEVEN);
  // io_write(I2C_io, I2C_example_str, 12);
// }

enum i2c_state_t {i2c_ts, i2c_ads1115 };
static enum i2c_state_t i2c_state = i2c_ts;

void i2c_poll(void) {
  enum i2c_state_t input_state = i2c_state;
  while (i2c_enabled && I2C_txfr_complete) {
    switch (i2c_state) {
      case i2c_ts:
        if (pm_poll()) {
          i2c_state = i2c_ads1115;
        }
        break;
      case i2c_ads1115:
        if (ads1115_poll()) {
          i2c_state = i2c_ts;
        }
        break;
      default:
        assert(false, __FILE__, __LINE__);
    }
    if (i2c_state == input_state) break;
  }
}

subbus_driver_t sb_i2c = {
  I2C_BASE_ADDR, I2C_HIGH_ADDR, // address range
  i2c_cache,
  i2c_reset,
  i2c_poll,
  0, // Dynamic function
  false
};
