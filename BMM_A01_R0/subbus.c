/* subbus.c for Atmel Studio
 */
#include "subbus.h"

static subbus_driver_t *drivers[SUBBUS_MAX_DRIVERS];
static int n_drivers = 0;

/** @return true on error.
 * Possible errors include too many drivers or drivers not in ascending order.
 */
bool subbus_add_driver(subbus_driver_t *driver) {
  if ((n_drivers >= SUBBUS_MAX_DRIVERS) ||
      ((n_drivers > 0) && (drivers[n_drivers-1]->high > driver->low)) ||
      driver->high < driver->low )
    return true;
  drivers[n_drivers++] = driver;
  return false;
}

void subbus_reset(void) {
  int i;
  for (i = 0; i < n_drivers; ++i) {
    if (drivers[i]->reset) {
      (*(drivers[i]->reset))();
    }
  }
}

void subbus_poll(void) {
  int i;
  for (i = 0; i < n_drivers; ++i) {
    if (drivers[i]->poll) {
      (*drivers[i]->poll)();
    }
  }
}

/**
 * @return non-zero on success (acknowledge)
 */
int subbus_read( uint16_t addr, uint16_t *rv ) {
  int i;
  for (i = 0; i < n_drivers; ++i) {
    if (addr < drivers[i]->low) return 0;
    if (addr <= drivers[i]->high) {
      uint16_t offset = addr-drivers[i]->low;
      subbus_cache_word_t *cache = &drivers[i]->cache[offset];
      if (cache->readable) {
        *rv = cache->cache;
        cache->was_read = true;
        return 1;
      }
    }
  }
  *rv = 0;
  return 0;
}

/**
 * @return non-zero on success (acknowledge)
 */
int subbus_write( uint16_t addr, uint16_t data) {
  int i;
  for (i = 0; i < n_drivers; ++i) {
    if (addr < drivers[i]->low) return 0;
    if (addr <= drivers[i]->high) {
      uint16_t offset = addr-drivers[i]->low;
      subbus_cache_word_t *cache = &drivers[i]->cache[offset];
      if (cache->writable) {
        cache->wvalue = data;
        cache->written = true;
        return 1;
      }
    }
  }
  return 0;
}

void set_fail(uint16_t arg) {
#if defined(SUBBUS_FAIL_ADDR)
  subbus_write(SUBBUS_FAIL_ADDR, arg);
#endif
}

#if SUBBUS_INTERRUPTS
extern volatile uint8_t subbus_intr_req;
void init_interrupts(void);
int intr_attach(int id, uint16_t addr);
int intr_detach( uint16_t addr );
void intr_service(void);
#endif

static subbus_cache_word_t sb_base_cache[SUBBUS_BDID_ADDR+2] = {
  { 0, 0, 0, 0, 0, 0 }, // Reserved zero address
  { 0, 0, 0, 0, 0, 0} , // INTA
  { SUBBUS_BUILD_NUM, 0, 1, 0, 0, 0 }, // Build number (SUBBUS_BDID_ADDR)
  { SUBBUS_BOARD_ID, 0, 1, 0, 0, 0 }      // Board ID
};

subbus_driver_t sb_base = { 0, SUBBUS_BDID_ADDR+1, sb_base_cache, 0, 0, false };

static subbus_cache_word_t sb_fail_sw_cache[SUBBUS_SWITCHES_ADDR-SUBBUS_FAIL_ADDR+1] = {
  { 0, 0, 1, 0, 1, 0 }, // Fail Register
  { 0, 0, 1, 0, 0, 0 }  // Switches
};

static void sb_fail_sw_reset() {
  sb_fail_sw_cache[0].cache = 0;
}

static void sb_fail_sw_poll() {
  if (sb_fail_sw_cache[0].written) {
    sb_fail_sw_cache[0].cache = sb_fail_sw_cache[0].wvalue;
    sb_fail_sw_cache[0].written = false;
  }
}

subbus_driver_t sb_fail_sw = { SUBBUS_FAIL_ADDR, SUBBUS_SWITCHES_ADDR,
    sb_fail_sw_cache, sb_fail_sw_reset, sb_fail_sw_poll, false };


/**
 * If a value has been written to the specified address since the
 * last call to this function, the new value is written at the
 * value address.
 * @param addr The cache address
 * @param value Pointer where value may be written
 * @return true if a value has been written to this address.
 */
bool subbus_cache_iswritten(subbus_driver_t *drv, uint16_t addr, uint16_t *value) {
  if (addr >= drv->low && addr <= drv->high) {
    subbus_cache_word_t *word = &drv->cache[addr-drv->low];
    if (word->writable && word->written) {
      *value = word->wvalue;
      word->written = false;
      return true;
    }
  }
  return false;
}
  
/**
 * This function differs from subbus_cache_write() in that it directly
 * updates the cache value. subbus_cache_write() is specifically for
 * write originating from the control port. subbus_cache_update() is
 * used by internal functions for storing data acquired from
 * peripherals, or for storing values written from the control
 * port after verifying them.
 * @param drv The driver structure
 * @param addr The cache address
 * @param data The value to be written
 * @return true on success
 */
bool subbus_cache_update(subbus_driver_t *drv, uint16_t addr, uint16_t data) {
  if (addr >= drv->low && addr <= drv->high) {
    subbus_cache_word_t *word = &drv->cache[addr-drv->low];
    if (word->readable) {
      word->cache = data;
      word->was_read = false;
      return true;
    }
  }
  return false;
}
