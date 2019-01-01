#ifndef SUBBUS_H_INCLUDED
#define SUBBUS_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

#define FCC3
#define USE_SUBBUS 1

#ifdef FCC1
  #define BOARD_REV                   "V7:178:HCHO FCC Rev A V1.1"
  #define SUBBUS_BOARD_ID             10
#endif
#ifdef FCC2
  #define BOARD_REV                   "V7:178:HCHO FCC Rev A V1.1"
  #define SUBBUS_BOARD_ID             11
#endif
#ifdef FCC3
  #define BOARD_REV                   "V7:178:Plant Chamber FCC Rev A V1.2"
  #define SUBBUS_BOARD_ID             13
#endif
#ifdef uDACS_12
#define BOARD_REV                   "V9:178:HCHO uDACS Rev A V1.1"
#define SUBBUS_BOARD_ID             12
#endif

#if USE_SUBBUS
#define SUBBUS_BUILD_NUM            1
#define SUBBUS_FAIL_RESERVED        0xF000
#define SUBBUS_INTA_ADDR            0x0001
#define SUBBUS_BDID_ADDR            0x0002
#define SUBBUS_FAIL_ADDR            0x0004
#define SUBBUS_SWITCHES_ADDR        0x0005
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
  uint16_t cache;
  uint16_t wvalue;
  bool readable;
  bool was_read;
  bool writable;
  bool written;
} subbus_cache_word_t;

typedef struct {
  uint16_t low, high;
  subbus_cache_word_t *cache;
  void (*reset)(void);
  void (*poll)(void);
  bool initialized;
} subbus_driver_t;

bool subbus_add_driver(subbus_driver_t *driver);
extern subbus_driver_t sb_base;
extern subbus_driver_t sb_fail_sw;

bool subbus_cache_iswritten(subbus_driver_t *drv, uint16_t addr, uint16_t *value);
bool subbus_cache_update(subbus_driver_t *drv, uint16_t addr, uint16_t data);

#endif // USE_SUBBUS

#endif
