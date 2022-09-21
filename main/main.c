
/*
    MIT License
    
    Copyright (c) 2022 Julian Scheffers
    
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include "main.h"

static pax_buf_t buf;
xQueueHandle buttonQueue;

#include <esp_log.h>
static const char *TAG = "pixie-fpga";

// FPGA binary.
extern const uint8_t fpga_bin_start[] asm("_binary_pixie_bin_start");
extern const uint8_t fpga_bin_end[]   asm("_binary_pixie_bin_end");

// Last recorded state of registers.
static uint16_t bus_peek[16];

// Updates the screen with the latest buffer.
void disp_flush() {
    ili9341_write(get_ili9341(), buf.buf);
}

// Exits the app, returning to the launcher.
void exit_to_launcher() {
    REG_WRITE(RTC_CNTL_STORE0_REG, 0);
    esp_restart();
}

void app_main() {
  
    
    ESP_LOGI(TAG, "Welcome to the template app!");

    // Initialize the screen, the I2C and the SPI busses.
    bsp_init();

    // Initialize the RP2040 (responsible for buttons, etc).
    bsp_rp2040_init();
    
    // This queue is used to receive button presses.
    buttonQueue = get_rp2040()->queue;
    
    // Initialize graphics for the screen.
    pax_buf_init(&buf, NULL, 320, 240, PAX_BUF_16_565RGB);
    
    // Initialize NVS.
    nvs_flash_init();
    
    // Initialize WiFi. This doesn't connect to Wifi yet.
    wifi_init();
    
    // Initialise the FPGA.
    bsp_ice40_init();
    ice40_load_bitstream(get_ice40(), fpga_bin_start, fpga_bin_end - fpga_bin_start);
    
    while (1) {
        spi_trns();
        redraw();
        
        // Thingy.
        disp_flush();
        rp2040_input_message_t message;
        xQueueReceive(buttonQueue, &message, portMAX_DELAY);
        if (message.input == RP2040_INPUT_BUTTON_HOME && message.state) {
            exit_to_launcher();
        }
    }
}

// Perform an SPI transaction.
void spi_trns() {
    // Perform the SPI transaction.
    uint8_t recv[32];
    uint8_t send[32];
    ice40_transaction(get_ice40(), send, sizeof(send), recv, sizeof(recv));
    
    // Interpret the bus peek.
    for (int i = 0; i < 16; i++) {
        bus_peek[i] = recv[i*2] | (recv[i*2+1] << 8);
    }
}

// Redraw all graphics.
void redraw() {
    pax_background(&buf, 0);
    
    // Show registers and busses.
    int peek_sel = 0;
    const char *peek_names[] = {
        "R0",   "R1",   "R2",  "R3",  "ST", "PF", "PC",
        "imm0", "imm1", "PbA", "PbB", "AR", "Db", "Ab",
    };
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 7; x++) {
            int peek_idx = x+y*7;
            pax_col_t name_col = peek_sel != peek_idx ? 0xff009fff : 0xff00ff9f;
            char tmp[5];
            snprintf(tmp, sizeof(tmp), "%04X", bus_peek[peek_idx]);
            pax_draw_text(&buf, name_col, pax_font_sky_mono, 9, x*buf.width/7.0, y*18, peek_names[peek_idx]);
            pax_draw_text(&buf, 0xffffffaf, pax_font_sky_mono, 9, x*buf.width/7.0, y*18+9, tmp);
        }
    }
    
    // Show display.
    for (int x = 0; x < 32; x++) {
        for (int y = 0; y < 16; y++) {
            pax_col_t col = ((x ^ y) & 1) ? 0xff009fff : 0xff000000;
            pax_draw_rect(&buf, col, x*10, y*10+40, 10, 10);
        }
    }
    
}
