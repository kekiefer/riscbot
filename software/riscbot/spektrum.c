#include <stdio.h>
#include <stdint.h>

#include "platform.h"
#include "encoding.h"

// #define DO_BIND 1
#define DSMX_11MS 9
#define DSMX_22MS 7
#define DSM2_11MS 5
#define DSM2_22MS 3

unsigned long long mtime_lo(void);
void use_hfrosc(int div, int trim);
void use_pll(int refsel, int bypass, int r, int f, int q);
void use_default_clocks();
void uart_init(size_t baud_rate);

uint16_t rc_data[7] = { 1024 };

extern void trap_entry();

typedef struct {
  uint8_t fades;
  uint8_t system;
  uint16_t servo[7];
} frame_data;

// #define DO_BIND
#define RX_POWER_GPIO_OFFSET 18
#define BIND_GPIO_OFFSET 16

#define SYSTEM_22MS_DSM2_1024 0x01 // 1024 bit
#define SYSTEM_11MS_DSM2_2048 0x12 // 2048 bit
#define SYSTEM_22MS_DSMX_2048 0xa2 // 2048 bit
#define SYSTEM_11MS_DSMX_2048 0xb2 // 2048 bit

#define MASK_1024_CHANID 0xFC00
#define SHIFT_1024_CHANID 10
#define MASK_1024_SXPOS 0x03FF

#define MASK_2048_CHANID 0x7800
#define SHIFT_2048_CHANID 11
#define MASK_2048_SXPOS 0x07FF


static void _putc(char c) {
  while ((int32_t) UART0_REG(UART_REG_TXFIFO) < 0);
  UART0_REG(UART_REG_TXFIFO) = c;
}

static int _getc(void* c){
  int32_t val = (int32_t) UART0_REG(UART_REG_RXFIFO);
  if (val & 0x80000000) {
    return 0;
  }

  *((char*)c) =  val & 0xFF;
  return 1;
}

static void _puts(const char * s) {
  while (*s != '\0'){
    _putc(*s++);
  }
}

void init_rc()
{
  // GPIO_REG(GPIO_OUTPUT_VAL) &= ~(0x1<< RX_POWER_GPIO_OFFSET);
  GPIO_REG(GPIO_OUTPUT_VAL) |= (0x1<< RX_POWER_GPIO_OFFSET);
  GPIO_REG(GPIO_OUTPUT_EN) |= (0x1<< RX_POWER_GPIO_OFFSET) ;

  uint64_t now = mtime_lo();
  while (mtime_lo() - now < 10000) ;

  /* enable receiver power */
  GPIO_REG(GPIO_OUTPUT_VAL) |= (0x1<< RX_POWER_GPIO_OFFSET);

#ifdef DO_BIND
  int bind_pulses = DSMX_11MS;

  GPIO_REG(GPIO_OUTPUT_EN) |= (0x1 << BIND_GPIO_OFFSET);
  GPIO_REG(GPIO_OUTPUT_VAL) |= (0x1<< BIND_GPIO_OFFSET);

  now = mtime_lo();
  while (mtime_lo() - now < 2400) ;

  // read the current value of the LEDS and invert them.
  uint32_t leds = GPIO_REG(GPIO_OUTPUT_VAL);

  while (bind_pulses--) {
    GPIO_REG(GPIO_OUTPUT_VAL) = leds & ~(0x1<< BIND_GPIO_OFFSET);

    now = mtime_lo();
    while (mtime_lo() - now < 38) ;

    GPIO_REG(GPIO_OUTPUT_VAL) = leds | (0x1<< BIND_GPIO_OFFSET);

    now = mtime_lo();
    while (mtime_lo() - now < 38) ;
  }

  GPIO_REG(GPIO_OUTPUT_EN) &= ~((0x1 << BIND_GPIO_OFFSET)) ;
#endif

  uart_init(115200); // 125000);

  char tmp;
  while (_getc(&tmp))
    ;

  printf("core freq at %d Hz\n", get_cpu_freq());
}

// #define PRINT_RAW

void read_rc_frame()
{
  static uint64_t next_frame = 0;

  if (mtime_lo() < next_frame)
  {
    char tmp;
    // flush uart fifo
    while (_getc(&tmp))
      ;
    return;
  }

#ifdef PRINT_RAW
  char c;
  if (!_getc(&c))
    return;

  printf("%x ", c);
#else
  frame_data data;
  char *pdata;
  pdata = (char*)data.servo;
  int i;

  if (!_getc(&data.fades))
    return;

  while (!_getc(&data.system))
    ;

  // frame starting, next frame will be at least 11ms from now
  next_frame = mtime_lo() + 8 * RTC_FREQ / 1000;

  switch (data.system)
  {
  // case SYSTEM_22MS_DSM2_1024:
  case SYSTEM_11MS_DSM2_2048:
  case SYSTEM_22MS_DSMX_2048:
  case SYSTEM_11MS_DSMX_2048:
    break;
  default:
    return;
  }

  while (pdata < ((&data)+1)) {
    while (!_getc(pdata))
      ;
    pdata++;
  }

  for (i = 0; i < 7; i++) {
    uint16_t raw = ((data.servo[i] & 0xff) << 8) | ((data.servo[i] >> 8) & 0xff);
    uint16_t channel = (raw & MASK_2048_CHANID) >> SHIFT_2048_CHANID;
    uint16_t value = raw & MASK_2048_SXPOS;
    rc_data[channel] = value;
  }

  printf("frame complete system: %x fades: %x 0:%d 1:%d 2:%d 3:%d 4:%d 5:%d 6:%d\r\n",
    data.system, data.fades,
    rc_data[0], rc_data[1], rc_data[2], rc_data[3], rc_data[4], rc_data[5], rc_data[6]
  );
#endif
}
