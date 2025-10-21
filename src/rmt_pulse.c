
/******************************************************************************/
/***        include files                                                   ***/
/******************************************************************************/

#include "rmt_pulse.h"

#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "esp_err.h"

/******************************************************************************/
/***        macro definitions                                               ***/
/******************************************************************************/

/******************************************************************************/
/***        type definitions                                                ***/
/******************************************************************************/

/******************************************************************************/
/***        local function prototypes                                       ***/
/******************************************************************************/

/**
 * @brief Remote peripheral interrupt. Used to signal when transmission is done.
 */
// static void IRAM_ATTR rmt_interrupt_handler(void *arg);

/******************************************************************************/
/***        exported variables                                              ***/
/******************************************************************************/

/******************************************************************************/
/***        local variables                                                 ***/
/******************************************************************************/

// static intr_handle_t gRMT_intr_handle = NULL;

static rmt_channel_handle_t row_rmt_channel;
static rmt_encoder_handle_t row_rmt_encoder;

/**
 * @brief keep track of wether the current pulse is ongoing
 */
// static volatile bool rmt_tx_done = true;

/******************************************************************************/
/***        exported functions                                              ***/
/******************************************************************************/

void rmt_pulse_init(gpio_num_t pin)
{
    if (row_rmt_channel) {
        return;
    }

    const rmt_tx_channel_config_t tx_channel_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = pin,
        .resolution_hz = 10000000, // 0.1us resolution (80MHz / 8)
        .mem_block_symbols = 64,
        .trans_queue_depth = 4,
        .flags = {
            .invert_out = 0,
            .with_dma = 0,
            .io_loop_back = 0,
        },
    };

    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_channel_config, &row_rmt_channel));

    const rmt_copy_encoder_config_t encoder_config = {};
    ESP_ERROR_CHECK(rmt_new_copy_encoder(&encoder_config, &row_rmt_encoder));

    ESP_ERROR_CHECK(rmt_enable(row_rmt_channel));
}


void IRAM_ATTR pulse_ckv_ticks(uint16_t high_time_ticks,
                               uint16_t low_time_ticks, bool wait)
{
    // while (!rmt_tx_done) ;

    if (!row_rmt_channel || !row_rmt_encoder) {
        return;
    }

    rmt_symbol_word_t symbol = {0};
    if (high_time_ticks > 0)
    {
        symbol.level0 = 1;
        symbol.duration0 = high_time_ticks;
        symbol.level1 = 0;
        symbol.duration1 = low_time_ticks;
    }
    else
    {
        symbol.level0 = 1;
        symbol.duration0 = low_time_ticks;
        symbol.level1 = 0;
        symbol.duration1 = 0;
    }

    static const rmt_transmit_config_t transmit_config = {
        .loop_count = 0,
        .flags = {
            .eot_level = 0,
            .queue_nonblocking = 0,
        },
    };

    esp_err_t err = rmt_transmit(row_rmt_channel, row_rmt_encoder, &symbol,
                                 sizeof(symbol), &transmit_config);
    if (err != ESP_OK)
    {
        return;
    }

    if (wait)
    {
        rmt_tx_wait_all_done(row_rmt_channel, -1);
    }
}


void IRAM_ATTR pulse_ckv_us(uint16_t high_time_us, uint16_t low_time_us, bool wait)
{
    pulse_ckv_ticks(10 * high_time_us, 10 * low_time_us, wait);
}


// bool IRAM_ATTR rmt_busy()
// {
//     return !rmt_tx_done;
// }

/******************************************************************************/
/***        local functions                                                 ***/
/******************************************************************************/

// static void IRAM_ATTR rmt_interrupt_handler(void *arg)
// {
//     rmt_tx_done = true;
//     RMT.int_clr.val = RMT.int_st.val;
// }

/******************************************************************************/
/***        END OF FILE                                                     ***/
/******************************************************************************/
