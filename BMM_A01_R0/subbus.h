#ifndef SUBBUS_H_INCLUDED
#define SUBBUS_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#include "serial_num.h"

#define USE_SUBBUS 1

#if USE_SUBBUS
#define SUBBUS_FAIL_RESERVED        0xF000
#define SUBBUS_INTA_ADDR            0x0001
#define SUBBUS_BDID_ADDR            0x0002
#define SUBBUS_BLDNO_ADDR           0x0003
#define SUBBUS_BDSN_ADDR            0x0004
#define SUBBUS_INSTID_ADDR          0x0005
#define SUBBUS_FAIL_ADDR            0x0006
#define SUBBUS_SWITCHES_ADDR        0x0007
#define SUBBUS_DESC_FIFO_SIZE_ADDR  0x0008
#define SUBBUS_DESC_FIFO_ADDR       0x0009
#define SUBBUS_MAX_DRIVERS          6
#define SUBBUS_INTERRUPTS           0

#define SUBBUS_ADDR_CMDS 0x18

#if SUBBUS_INTERRUPTS
extern volatile uint8_t subbus_intr_req;
void init_interrupts(void);
int intr_attach(int id, uint16_t addr);
int intr_detach( uint16_t addr );
void intr_service(void);
#endif
int subbus_read( uint16_t addr, uint16_t *rv );
int subbus_write( uint16_t addr, uint16_t data);
void subbus_reset(void);
void subbus_poll(void);
void set_fail(uint16_t arg);

typedef struct {
  /** The current value of this word */
  uint16_t cache;
  /** The value that has been written. Allows the driver code to do checks for validity */
  uint16_t wvalue;
  /** True if this word is readable */
  bool readable;
  /** True if this word has been read */
  bool was_read;
  /** True if this word is writable */
  bool writable;
  /** True if this word has been written */
  bool written;
  /** True to invoke sb_action immediately rather than waiting for poll */
  bool dynamic;
} subbus_cache_word_t;

typedef struct {
  uint16_t low, high;
  subbus_cache_word_t *cache;
  void (*reset)(void);
  void (*poll)(void);
  void (*sb_action)(void); // called if dynamic
  bool initialized;
} subbus_driver_t;

bool subbus_add_driver(subbus_driver_t *driver);
extern subbus_driver_t sb_base;
extern subbus_driver_t sb_fail_sw;

bool subbus_cache_iswritten(subbus_driver_t *drv, uint16_t addr, uint16_t *value);
bool subbus_cache_was_read(subbus_driver_t *drv, uint16_t addr);
bool subbus_cache_update(subbus_driver_t *drv, uint16_t addr, uint16_t data);

#endif // USE_SUBBUS

#endif
