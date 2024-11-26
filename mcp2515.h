#include <stdint.h>

#ifndef MCP2515_H
#define MCP2515_H

extern const uint32_t SPEED;

uint8_t tx_buffer[14]; // Instruction byte + all the necessary MCP2515 registers
uint8_t rx_buffer[14];
struct spi_ioc_transfer trx;
int dev_file;
int dev_response;
const uint32_t SPI_MODE = (0|0);

struct can_frame {
    uint16_t sid; // 0 <= sid <= 0x7FF
    uint8_t n_bytes; // 0 <= n_bytes <= 8
    uint8_t* data; // Array of 8 bytes
};

int initialize();
int reset;
int deinitialize();
void print_buffers();
int transmit_can_frame(struct can_frame* frame);
struct can_frame* read_can_rx();
void del_frame();
void clear_tx_buffer();
void print_frame();

#endif // MCP2515_H