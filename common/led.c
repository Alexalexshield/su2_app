#include "system.h"

#ifdef MU_APP
#include "..\mu2_app\mu_def.h"
#else
#define LED_POWER OFF
#define LED_POWER_ON
#endif

///////////////////// STATE AND COUNTERS FOR ALERT BEEPER //////
volatile long g_led_on_ms=0;
volatile long g_led_off_ms = 0;
volatile long g_led_counter = 0;
volatile int g_led_times=0;
#define LED_STATE_DISABLED 0
#define LED_STATE_ON 1
#define LED_STATE_OFFCOUNTDOWN 2
volatile int g_led_state = LED_STATE_DISABLED;
#define LED_ON_PERIOD_MS 100
#define LED_OFF_PERIOD_MS 900
///////////////////////////////////////////////////////////////

void led_on(int times)
{
	g_led_on_ms = LED_ON_PERIOD_MS;
	g_led_off_ms = LED_OFF_PERIOD_MS;
	g_led_times = times;
	g_led_counter = 0;
	g_led_state = LED_STATE_ON;
	LED_POWER_ON;

}

// call this every ms
void ProcessLEDTasks()
{
	if (g_led_state == LED_STATE_DISABLED) return;
	
	g_led_counter++;
	if (g_led_state == LED_STATE_ON) {
		if (g_led_counter >= g_led_on_ms) {
			g_led_state = LED_STATE_OFFCOUNTDOWN;
			g_led_counter = 0;
			LED_POWER_OFF;
		}
	}
	else if (g_led_state == LED_STATE_OFFCOUNTDOWN)
	{
		if (g_led_counter >= g_led_off_ms) {
			g_led_counter = 0;
			if (g_led_times) {
				g_led_times--;
			}
			if (g_led_times) {
				g_led_state = LED_STATE_ON;
				LED_POWER_ON;
			}
			else {
				LED_POWER_OFF;
				g_led_state = LED_STATE_DISABLED;
			}
		}
	}
	
}




