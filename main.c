#include "asf.h"
#include "buttons.h"
#include "io_capture.h"
#include "udc.h"
#include "udi_cdc.h"
#include "commands.h"
#include "lcd.h"
#include "SSD1306_commands.h"


/** Set default LED blink period to 250ms*3 */
#define DEFAULT_LED_FREQ   4

/** LED blink period */
#define LED_BLINK_PERIOD    3

/** LED blink period */
static volatile uint32_t led_blink_period = 0;
volatile bool g_interrupt_enabled = true;

/**
 *  \brief Handler for System Tick interrupt.
 *
 *  Process System Tick Event
 */
void SysTick_Handler(void)
{
}

/**
 * \brief Interrupt handler for TC0 interrupt. Toggles the state of LEDs.
 */
void TC0_Handler(void)
{
    /* Clear status bit to acknowledge interrupt */
    tc_get_status(TC0, 0);

    led_blink_period++;

    if (led_blink_period == LED_BLINK_PERIOD) {
        int buttons = btn_state();

        if(buttons)
            uart_write(UART0, buttons + 0x30);

        pio_toggle_pin(PIO_PA8_IDX);
        led_blink_period = 0;
    }
}

/**
 * \brief Configure Timer Counter 0 to generate an interrupt with the specific
 * frequency.
 *
 * \param freq Timer counter frequency.
 */
static void configure_tc(uint32_t freq)
{
    uint32_t ul_div;
    uint32_t ul_tcclks;
    uint32_t ul_sysclk = sysclk_get_cpu_hz();

    /* Disable TC first */
    tc_stop(TC0, 0);
    tc_disable_interrupt(TC0, 0, TC_IER_CPCS);

    /** Configure TC with the frequency and trigger on RC compare. */
    tc_find_mck_divisor(freq, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
    tc_init(TC0, 0, ul_tcclks | TC_CMR_CPCTRG);
    tc_write_rc(TC0, 0, (ul_sysclk / ul_div) / 4);

    /* Configure and enable interrupt on RC compare */
    NVIC_EnableIRQ((IRQn_Type)ID_TC0);
    tc_enable_interrupt(TC0, 0, TC_IER_CPCS);

    /** Start the counter. */
    tc_start(TC0, 0);
}

static void configure_console(void)
{
    sam_uart_opt_t uart_settings;
    uart_settings.ul_mck = sysclk_get_peripheral_hz();
    uart_settings.ul_baudrate = 115200Ul;
    uart_settings.ul_mode = UART_MR_PAR_NO;

    pio_configure(PIOA, PIO_PERIPH_A, (PIO_PA9A_URXD0 | PIO_PA10A_UTXD0), PIO_DEFAULT);
    sysclk_enable_peripheral_clock(ID_UART0);
    uart_init(UART0, &uart_settings);
}

/**
 * \brief Configure timer ISR to fire regularly.
 */
static void init_timer_isr(void)
{
    SysTick_Config((sysclk_get_cpu_hz() / 1000) * 100); // 100 ms
}

/**
 * \brief initialize pins, watchdog, etc.
 */
static void init_system(void)
{
    /* Disable the watchdog */
    wdt_disable(WDT);

    irq_initialize_vectors();
    cpu_irq_enable();

    /* Initialize the system clock */
    sysclk_init();

    /* Configure LED pins */
    pio_configure(PIOA, PIO_OUTPUT_0, (PIO_PA7 | PIO_PA8), PIO_DEFAULT);

    /* Enable PMC clock for key/slider PIOs  */
    pmc_enable_periph_clk(ID_PIOA);
    pmc_enable_periph_clk(ID_PIOB);

    /* Configure PMC */
    pmc_enable_periph_clk(ID_TC0);

    /* Configure the default TC frequency */
    configure_tc(DEFAULT_LED_FREQ);

    /* Enable the serial console */
    configure_console();

    btn_init();
    SSD1306_init();

    ioc_init();
    ioc_set_clock(F1MHZ);

    /* Start USB stack to authorize VBus monitoring */
    udc_start();
}

uint8_t samples[32];

void ioc_show(void* param) {
    uint8_t* data = (uint8_t*)(param);

    for(int i = 0; i < 32; ++i) {
        uart_write(UART0, 0x30 + data[i]);
        for(int j = 0; j < 65535; ++j) __NOP;
    }
}



int main(void)
{
    const uint8_t *cmd_resp;
    uint8_t i, j = 0;

    /* Initialize the SAM system */
    init_system();

    /* Configure timer ISR to fire regularly */
    init_timer_isr();

    uart_write(UART0, 'U');

    // TODO remove
    pio_configure(PIOA, PIO_OUTPUT_0, (PIO_PA20 | PIO_PA22), PIO_DEFAULT);
    ioc_set_handler(ioc_show, samples);
    ioc_fetch(samples, 32);

    uint8_t siema[5]= "siema";
//    SSD1306_setString(0,0,siema, 5,1);
//    SSD1306_drawBitmap();
////
//	siema[5]= "abcde";

    if(!SSD1306_isBusy())
    {
		SSD1306_setString(0,0,siema, 5,1);
		SSD1306_setLine(15, 15, 20, 60,0);
		SSD1306_drawBitmapDMA();
    }

    /* Loop forever */
    for (;;) {
        if (udi_cdc_is_rx_ready()) {
            cmd_new_data(udi_cdc_getc());
            cmd_resp = cmd_try_execute();

            if (cmd_resp) {
                for (i = 0; i < cmd_raw_len(cmd_resp); ++i) {
                    udi_cdc_putc(cmd_resp[i]);
                    uart_write(UART0, cmd_resp[i]); // TODO remove
                }
            }
        }

        pio_toggle_pin(PIO_PA7_IDX);

        if(j % 10 == 0)
            pio_toggle_pin(PIO_PA20_IDX);

        if(j % 30 == 0)
            pio_toggle_pin(PIO_PA22_IDX);

        ++j;
    }
}
