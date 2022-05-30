/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011,2012 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ch.h"
#include "hal.h"
#include "test.h"
#include "shell.h"
#include "chprintf.h"

#define SHELL_WA_SIZE       THD_WA_SIZE(4096)

#ifdef EXTENDED_SHELL

#define TEST_WA_SIZE        THD_WA_SIZE(4096)

static void cmd_mem(BaseSequentialStream *chp, int argc, char *argv[]) {
  size_t n, size;

  UNUSED(argv);
  if (argc > 0) {
    chprintf(chp, "Usage: mem\r\n");
    return;
  }
  n = chHeapStatus(NULL, &size);
  chprintf(chp, "core free memory : %u bytes\r\n", chCoreStatus());
  chprintf(chp, "heap fragments   : %u\r\n", n);
  chprintf(chp, "heap free total  : %u bytes\r\n", size);
}

static void cmd_threads(BaseSequentialStream *chp, int argc, char *argv[]) {
  static const char *states[] = {THD_STATE_NAMES};
  Thread *tp;

  UNUSED(argv);
  if (argc > 0) {
    chprintf(chp, "Usage: threads\r\n");
    return;
  }
  chprintf(chp, "    addr    stack prio refs     state time    name\r\n");
  tp = chRegFirstThread();
  do {
    chprintf(chp, "%.8lx %.8lx %4lu %4lu %9s %-8lu %s\r\n",
            (uint32_t)tp, (uint32_t)tp->p_ctx.r13,
            (uint32_t)tp->p_prio, (uint32_t)(tp->p_refs - 1),
			 states[tp->p_state], (uint32_t)tp->p_time, tp->p_name);
    tp = chRegNextThread(tp);
  } while (tp != NULL);
}

static void cmd_test(BaseSequentialStream *chp, int argc, char *argv[]) {
  Thread *tp;

  UNUSED(argv);
  if (argc > 0) {
    chprintf(chp, "Usage: test\r\n");
    return;
  }
  tp = chThdCreateFromHeap(NULL, TEST_WA_SIZE, chThdGetPriority(),
                           TestThread, chp);
  if (tp == NULL) {
    chprintf(chp, "out of memory\r\n");
    return;
  }
  chThdWait(tp);
}

#endif // EXTENDED_SHELL

static void cmd_reboot(BaseSequentialStream *chp, int argc, char *argv[]) {
  UNUSED(argv);
  if (argc > 0) {
    chprintf(chp, "Usage: reboot\r\n");
    return;
  }

  /* Watchdog will cause reset after 1 tick.*/
  watchdog_start(1);
}

static const ShellCommand commands[] = {
#ifdef EXTENDED_SHELL
  {"mem", cmd_mem},
  {"threads", cmd_threads},
  {"test", cmd_test},
#endif
  {"reboot", cmd_reboot},
  {NULL, NULL}
};

static const ShellConfig shell_config = {
  (BaseSequentialStream *)&SD1,
  commands
};

#if 1
// This demo blinks the LED on the PWM pin (GPIO 18) 5 times per second while
// changing duty-cycle every 2 seconds. Duty cycle starts at 55%, then increases
// by 10% every 2 seconds, eventually reaching 95%, then wrapping around to
// 5% after another 2 seconds, then repeating the whole sequence indefinitely.
// This is a good way to visually confirm the operation of the PWM driver+pin+etc,
// and although the intensity of the duty cycle can be hard to see precisely
// with just a LED, the change between the extremes and the progression can
// be visually seen. Of course, most things done with PWM will occur at
// frequencies that are imperceivable to naked human senses, so this example
// is not useful for much more than blinken-LED confirmation demo kinda stuff.
static PWMConfig pwm_config = {
  600000,      // PWM clock frequency; max is 19.2MHz. (Ignored by ChibiOS-RPi's original pwm_lld.c, which used 600kHz by default. Now adjustable.)
  120000,      // PWM period that driver starts with when pwmStart is called. `clock_freq / period = pwm_freq`.
  // Note that (PWM period * denominator) should not exceed 2^32, and for PWM_PERCENTAGE_TO_WIDTH, denominator is 10000.
  // Also note that the current BCM2835 ChibiOS fork hard-codes the PWM driver
  // to run with a sample rate of 600kHz. So if you set period to 600000 and
  // are using M-S mode (as below), then you'd get 1-second periods.
  // Likewise, set this to 120000, and your "range" (or S-value) will be set to
  // 120000 samples, which is 1/5 of the 600000 samples/second, thus the
  // period for the waveform will be 0.2s and the frequency will be 5Hz.
  // Thus, decreasing period sample-count will increase PWM frequency whlie
  // decreasing duty-cycle resolution.
  // If higher frequencies or duty-cycle resolutions are required, consider
  // either figuring out N-M mode (PWM_CTL &= ~PWM0_MODE_MS;) or try changing
  // the clock divider in `ChibiOS-RPi/os/hal/platforms/BCM2835/pwm_lld.c`.
  NULL,        // Period callback.
  {
   {PWM_OUTPUT_ACTIVE_HIGH, NULL}
  }
};

static WORKING_AREA(waThread1, 128);
static msg_t Thread1(void *p) {
  (void)p;
  chRegSetThreadName("blinker");

  // `duty_cycle` is an integer from 0 - 10000, with 10000 meaning 100%.
  // The initial value here is only initial... this can get modified by
  // code later on as the demo progresses.
  uint32_t  duty_cycle = 5500;
  
  pwmStart(&PWMD1, &pwm_config);
  PWM_CTL |= PWM0_MODE_MS;
  pwmEnableChannel(&PWMD1, 0, PWM_PERCENTAGE_TO_WIDTH(&PWMD1, duty_cycle));
  
  while (TRUE) {
    palClearPad(ONBOARD_LED_PORT, ONBOARD_LED_PAD);
    chThdSleepMilliseconds(200);
    palSetPad(ONBOARD_LED_PORT, ONBOARD_LED_PAD);
    chThdSleepMilliseconds(1800);

	// Change duty-cycle.
	pwmDisableChannel(&PWMD1, 0);
	duty_cycle += 1000;
	if ( duty_cycle >= 10000 )
		duty_cycle = 500;
	pwmEnableChannel(&PWMD1, 0, PWM_PERCENTAGE_TO_WIDTH(&PWMD1, duty_cycle));
  }
  
  pwmDisableChannel(&PWMD1, 0);
  pwmStop(&PWMD1);
  return 0;
}
#endif

#if 0
// This demo does a frequency sweep with PWM on GPIO18.
// 
// It uses 50% duty cycle always.
// It starts with a waveform at 21.5kHz, then sweeps up to 24.5kHz.
//
// These are the tolerances on the muRata MZB3004T04 micropump that
// I am aiming to drive with this PWM, hence that specific band of frequencies.
// (Note that the raw PWM signal coming from a logic pin wouldn't be able to
// drive such a pump, but I do intend to send that through a MOSFET to switch
// a higher voltage (ex: 12V) power source, then pass that power signal
// through something like an LC filter to smooth the waveform into something
// more sinusoidal, so that the pump gets the power signal that it wants.)
// 
// In playing with this, I also discovered that the clock frequency behind
// the PWM circuit shouldn't be set to 19.2MHz. For some reason that causes
// a very low frequency PWM, like a few Hz (but it was still frequency-sweeping,
// so the code wasn't just confused with the other demo). I set it to 9.6MHz
// instead, and everything worked fine. Dividing it by 8 also worked, but
// generally it is useful to use the highest clock possible, since that gives
// the resulting PWM waveform the highest fidelity.
//
static PWMConfig pwm_config = {
  9600000,     // PWM clock frequency; max is 19.2MHz. (Ignored by ChibiOS-RPi's original pwm_lld.c, which used 600kHz by default. Now adjustable.)
  893,         // PWM period that driver starts with when pwmStart is called. `clock_freq / period = pwm_freq`.
  NULL,        // Period callback.
  {
   {PWM_OUTPUT_ACTIVE_HIGH, NULL}
  }
};

static void set_pwm_config_freq_on_bcm2835(PWMConfig *config, uint32_t target_freq)
{
	config->period = config->frequency / target_freq;
}

static WORKING_AREA(waThread1, 128);
static msg_t Thread1(void *p) {
	(void)p;
	chRegSetThreadName("blinker");

	// `duty_cycle` is an integer from 0 - 10000, with 10000 meaning 100%.
	uint32_t  duty_cycle = 5000;

	// muRata MZB3004T04 resonant frequency range is 21.5kHz to 24.5kHz.
	// So we'll start at the bottom of that range and sweep up to the top.
	uint32_t freq = 21500; // in Hz
	set_pwm_config_freq_on_bcm2835(&pwm_config, freq);

	pwmStart(&PWMD1, &pwm_config);
	PWM_CTL |= PWM0_MODE_MS;
	pwmEnableChannel(&PWMD1, 0, PWM_PERCENTAGE_TO_WIDTH(&PWMD1, duty_cycle));

	while (TRUE) {
		palClearPad(ONBOARD_LED_PORT, ONBOARD_LED_PAD);
		chThdSleepMilliseconds(200);
		palSetPad(ONBOARD_LED_PORT, ONBOARD_LED_PAD);
		chThdSleepMilliseconds(1800);

		// Change frequency.
		pwmDisableChannel(&PWMD1, 0);
		pwmStop(&PWMD1);

		freq += 250;
		if ( freq > 24500 )
			freq = 21500;

		set_pwm_config_freq_on_bcm2835(&pwm_config, freq);
		BaseSequentialStream *output_stream = (BaseSequentialStream *)&SD1;
		chprintf(output_stream, "pwm_config->period   == %d\r\n", pwm_config.period);
		chprintf(output_stream, "pwm target frequency == %d\r\n", freq);
		chprintf(output_stream, "pwm result frequency == %d\r\n", pwm_config.frequency / pwm_config.period);

		pwmStart(&PWMD1, &pwm_config);
		PWM_CTL |= PWM0_MODE_MS;
		pwmEnableChannel(&PWMD1, 0, PWM_PERCENTAGE_TO_WIDTH(&PWMD1, duty_cycle));
	}

	pwmDisableChannel(&PWMD1, 0);
	pwmStop(&PWMD1);
	return 0;
}
#endif

/*
 * Application entry point.
 */
int main(void) {
  halInit();
  chSysInit();

  /*
   * Serial port initialization.
   */
  sdStart(&SD1, NULL);
  BaseSequentialStream *output_stream = (BaseSequentialStream *)&SD1;
  chprintf(output_stream, "Main (SD1 started)\r\n");

  /*
   * Shell initialization.
   */
  shellInit();
  shellCreate(&shell_config, SHELL_WA_SIZE, NORMALPRIO + 1);

  /*
   * PWM initialization.
   */
  //palSetPadMode(GPIO12_PORT, GPIO12_PAD, PAL_MODE_ALTERNATE(0));
  pwmInit();
  pwmObjectInit(&PWMD1);

  /*
   * Set mode of onboard LED
   */
  palSetPadMode(ONBOARD_LED_PORT, ONBOARD_LED_PAD, PAL_MODE_OUTPUT);

  /*
   * Creates the blinker thread.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

  /*
   * Events servicing loop.
   */
  chThdWait(chThdSelf());

  return 0;
}
