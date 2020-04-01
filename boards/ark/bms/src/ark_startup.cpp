#include <board_config.h>
#include <drivers/drv_hrt.h>
#include <lib/drivers/smbus/SMBus.hpp>
#include "ssd1306.h"
#include <uORB/topics/battery_status.h>

#define BATT_SMBUS_ADDR                                 0x0B
#define BATT_SMBUS_VOLTAGE                              0x09
#define BATT_SMBUS_CURRENT                              0x0A
#define BATT_SMBUS_RELATIVE_SOC							0x0D

static SMBus* _interface;

static float _voltage_mv = 0;
static float _current_ma = 0;
static float _soc = 0;

// Expose these functions for use in init.c
__BEGIN_DECLS
extern void ark_display_bq_startup_init(void);
extern void ark_check_button_and_update_display();
extern void ark_clean_up();
__END_DECLS

void update_battery_data(void);
void update_led_status(void);



void ark_display_bq_startup_init(void)
{
	int address = BATT_SMBUS_ADDR;
	int bus = 1;

	// Create the SMBus interface and initialize it
	_interface = new SMBus(bus, address);
	_interface->init();

	// Create the SPI interface and initialize it
	ssd1306::init();

	up_udelay(10000);
}

void ark_clean_up(void)
{
	delete _interface;
	ssd1306::clean_up();
}

void ark_check_button_and_update_display()
{
	static constexpr uint64_t BUTTON_SHUTDOWN_DURATION_US = 3000000;

	unsigned times_thru_loop = 0;
	unsigned button_still_held = 0;

	auto start_time = hrt_absolute_time();
	auto time_now = start_time;

	// Button should be held for 2.5 out of 3 seconds
	while (time_now < (start_time + BUTTON_SHUTDOWN_DURATION_US)) {
		update_battery_data();
		ssd1306::updateStatus(_voltage_mv, _current_ma, _soc);
		update_led_status();

		up_udelay(50000); // 100ms

		bool button_held = !stm32_gpioread(GPIO_BUTTON);

		if (button_held) {
			printf("Keep holding that button big guy\n");
			button_still_held++;
		}

		times_thru_loop++;
		time_now = hrt_absolute_time();
	}

	unsigned percent_held = (100 * button_still_held) / times_thru_loop;

	if (percent_held >= 90) {
		// Good to go
		printf("Congrats you held the button for %d percent of the time\n", percent_held);
		return;
	} else {
		printf("Button not held, powering off\n");

		ssd1306::displayOff();
		stm32_gpiowrite(GPIO_LED_5, true);
		stm32_gpiowrite(GPIO_LED_4, true);
		stm32_gpiowrite(GPIO_LED_3, true);
		stm32_gpiowrite(GPIO_LED_2, true);
		stm32_gpiowrite(GPIO_LED_1, true);
		stm32_gpiowrite(GPIO_PWR_EN, false);
		while(1){;};
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

void update_battery_data(void)
{
	uint16_t result = 0;

	// Get the voltage
	_interface->read_word(BATT_SMBUS_VOLTAGE, result);
	_voltage_mv = ((float)result) / 1000.0f;

	// Get the current
	_interface->read_word(BATT_SMBUS_CURRENT, result);
	_current_ma = result;

	// Get the state of charge
	_interface->read_word(BATT_SMBUS_RELATIVE_SOC, result);
	_soc = ((float)result) / 100.0f;
}

void update_led_status(void)
{
	float remaining = _soc; // 0 - 1
	int remaining_fifth = (int)(remaining * 5 + 1);

	switch (remaining_fifth)
	{
		case 1:
			stm32_gpiowrite(GPIO_LED_5, false);
			stm32_gpiowrite(GPIO_LED_4, true);
			stm32_gpiowrite(GPIO_LED_3, true);
			stm32_gpiowrite(GPIO_LED_2, true);
			stm32_gpiowrite(GPIO_LED_1, true);
			break;
		case 2:
			stm32_gpiowrite(GPIO_LED_5, false);
			stm32_gpiowrite(GPIO_LED_4, false);
			stm32_gpiowrite(GPIO_LED_3, true);
			stm32_gpiowrite(GPIO_LED_2, true);
			stm32_gpiowrite(GPIO_LED_1, true);
			break;
		case 3:
			stm32_gpiowrite(GPIO_LED_5, false);
			stm32_gpiowrite(GPIO_LED_4, false);
			stm32_gpiowrite(GPIO_LED_3, false);
			stm32_gpiowrite(GPIO_LED_2, true);
			stm32_gpiowrite(GPIO_LED_1, true);
			break;
		case 4:
			stm32_gpiowrite(GPIO_LED_5, false);
			stm32_gpiowrite(GPIO_LED_4, false);
			stm32_gpiowrite(GPIO_LED_3, false);
			stm32_gpiowrite(GPIO_LED_2, false);
			stm32_gpiowrite(GPIO_LED_1, true);
			break;
		case 5:
			stm32_gpiowrite(GPIO_LED_5, false);
			stm32_gpiowrite(GPIO_LED_4, false);
			stm32_gpiowrite(GPIO_LED_3, false);
			stm32_gpiowrite(GPIO_LED_2, false);
			stm32_gpiowrite(GPIO_LED_1, false);
			break;
	}
}