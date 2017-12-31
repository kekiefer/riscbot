#include <stdio.h>
#include <stdint.h>

#include "platform.h"
#include "encoding.h"

void use_default_clocks();
void use_pll(int refsel, int bypass, int r, int f, int q);
unsigned long long mtime_lo(void);

void init_rc();
void read_rc_frame();

extern void trap_entry();

extern uint16_t rc_data[7];

#define PWM0_OFFSET (19)
#define PWM1_OFFSET (20)
#define PWM2_OFFSET (21)
#define PWM3_OFFSET (22)

#define LED_STAT2_OFFSET (0)
#define LED_STAT1_OFFSET (1)

void led_init()
{
  PWM1_REG(PWM_CFG)   = 0;
  PWM1_REG(PWM_COUNT) = 0;

  PWM1_REG(PWM_CMP0)  = 0;
  PWM1_REG(PWM_CMP1)  = 0;
  PWM1_REG(PWM_CMP2)  = 0;
  PWM1_REG(PWM_CMP3)  = 0;

  PWM1_REG(PWM_CFG)   = (PWM_CFG_ENALWAYS) | (PWM_CFG_DEGLITCH) | 4;

  GPIO_REG(GPIO_IOF_EN )    |= (1 << PWM0_OFFSET) | (1 << PWM1_OFFSET) | (1 << PWM2_OFFSET) | (1 << PWM3_OFFSET);
  GPIO_REG(GPIO_OUTPUT_XOR) |= (1 << PWM0_OFFSET) | (1 << PWM1_OFFSET) | (1 << PWM2_OFFSET) | (1 << PWM3_OFFSET);
  GPIO_REG(GPIO_IOF_SEL)    |= (1 << PWM0_OFFSET) | (1 << PWM1_OFFSET) | (1 << PWM2_OFFSET) | (1 << PWM3_OFFSET);
}

int set_vector(int fwd_rev, int lft_rgt)
{
  int m1, m2;

  m1 = fwd_rev + lft_rgt;
  m2 = fwd_rev - lft_rgt;

  if (m1 > 682)
    m1 = 682;
  else if (m1 < -682)
    m1 = -682;

  if (m2 > 682)
    m2 = 682;
  else if (m2 < -682)
    m2 = -682;

  int m1_16 = 65535 * m1 / 682;

  if (m1_16 < 0) {
    PWM1_REG(PWM_CMP0) = abs(m1_16);
    PWM1_REG(PWM_CMP1) = 0;
  }
  else {
    PWM1_REG(PWM_CMP0) = 0;
    PWM1_REG(PWM_CMP1) = abs(m1_16); // PWM is low on the left, GPIO is low on the left side, LED is ON on the left.
  }

  int m2_16 = 65535 * m2 / 682;

  if (m2_16 < 0) {
    PWM1_REG(PWM_CMP2) = 0;
    PWM1_REG(PWM_CMP3) = abs(m2_16); // PWM is low on the left, GPIO is low on the left side, LED is ON on the left.
  }
  else {
    PWM1_REG(PWM_CMP2) = abs(m2_16);
    PWM1_REG(PWM_CMP3) = 0;
  }
}

gpio_init()
{
  GPIO_REG(GPIO_OUTPUT_VAL) = 0;

  GPIO_REG(GPIO_INPUT_EN) = 0;
  GPIO_REG(GPIO_OUTPUT_EN) = (0x1 << LED_STAT1_OFFSET) | (0x1 << LED_STAT2_OFFSET);

  GPIO_REG(GPIO_OUTPUT_VAL) |= (0x1 << LED_STAT1_OFFSET);
}

int main()
{
  use_default_clocks();
  use_pll(0, 0, 1, 31, 1);

  write_csr(mtvec, &trap_entry);
  if (read_csr(misa) & (1 << ('F' - 'A'))) { // if F extension is present
    write_csr(mstatus, MSTATUS_FS); // allow FPU instructions without trapping
    write_csr(fcsr, 0); // initialize rounding mode, undefined at reset
  }

  gpio_init();

  init_rc();

  led_init();

  while (1)
  {
    read_rc_frame();

#if 0
    // 342 - 1706
    // 1024 +/- 682
    int G = 65535 * (rc_data[0] - 1024) / 682;
    if (G < 0) {
      PWM1_REG(PWM_CMP0) = 0;
      PWM1_REG(PWM_CMP1) = abs(G); // PWM is low on the left, GPIO is low on the left side, LED is ON on the left.
    }
    else {
      PWM1_REG(PWM_CMP0)  = abs(G);
      PWM1_REG(PWM_CMP1) = 0;
    }
#else
    set_vector(rc_data[2] - 1024, -(rc_data[1] - 1024));
#endif
  }

  return 0;
}
