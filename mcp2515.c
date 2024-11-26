#include "mcp2515.h"

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#define SPI_DEVICE "/dev/spidev0.0"

const uint32_t SPEED = 10000000; // 10 MHz SPI


int initialize() {
    trx.rx_buf = rx_buffer;
    trx.tx_buf = tx_buffer;
    trx.len = sizeof(tx_buffer);
    trx.speed_hz = SPEED;
    trx.delay_usecs = 0;
    trx.bits_per_word = 0;

    dev_file = open(SPI_DEVICE, O_RDWR);
    if (dev_file < 0) {
        printf("Could not open SPI device\n");
        return -1;
    }

    dev_response = ioctl(dev_file, SPI_IOC_WR_MODE32, &SPI_MODE);
    if (dev_response != 0) {
        printf("Could not set SPI mode\n");
        close(dev_file);
        return -1;
    }

    dev_response = ioctl(dev_file, SPI_IOC_WR_MAX_SPEED_HZ, &SPEED);
    if (dev_response != 0) {
        printf("Could not set SPI speed\n");
        close(dev_file);
        return -1;
    }
    
    print_buffers();
    printf("Resetting MCP2515\n");
    reset();
    print_buffers();

    // Change from configuration mode to normal mode
    clear_tx_buffer();
    trx.len = 3; // Instruction, address, one byte of data
    tx_buffer[0] = 0b00000010; // WRITE
    tx_buffer[1] = 0x0F; // CANCTRL
    tx_buffer[2] = 0; // All zeros :3
    // tx_buffer[2] = 0x40; // Uncomment for loopback mode (works)
    ioctl(dev_file, SPI_IOC_MESSAGE(1), &trx);
    // Reset RXB0CTRL
    trx.len = sizeof(tx_buffer);
    tx_buffer[1] = 0x60; // RXB0CTRL
    tx_buffer[2] = 0x60; // Turn off filters
    for (int i = 3; i < 13; i++) {
    	tx_buffer[i] = 66 + i;
    }
    ioctl(dev_file, SPI_IOC_MESSAGE(1), &trx);
}

int reset() {
    tx_buffer[0] = 0b11000000; // RESET instruction
    dev_response = ioctl(dev_file, SPI_IOC_MESSAGE(1), &trx);
    return dev_response;
}

int deinitialize() {
    return close(dev_file);
}

void print_buffers() {
    printf("RX: [");
    for (int i = 0; i < sizeof(rx_buffer) - 1; i++) {
        printf("%d, ", rx_buffer[i]);
    }
    printf("%d]\n", rx_buffer[sizeof(rx_buffer) - 1]);
  
    printf("TX: [");
    for (int i = 0; i < sizeof(tx_buffer) - 1; i++) {
        printf("%d, ", tx_buffer[i]);
    }
    printf("%d]\n", tx_buffer[sizeof(tx_buffer) - 1]);
}

int transmit_can_frame(struct can_frame* frame) {
// int transmit_can_frame(uint16_t sid, uint8_t n_bytes, uint8_t* data) {
    // Sanity checks
    if (frame->n_bytes > 8) {
        // CAN Frame can have up to 8 bytes of data
        return -1;
    } 
    if (frame->sid > 0x7FF) {
        // Thank you random Bosch engineer for deciding on 11-bit SIDs
        return -2;
    }

    clear_tx_buffer();

    tx_buffer[0] = 0b01000000; // LOAD TX BUFFER TXB0, start at TXB0SIDH
    tx_buffer[1] = (uint8_t) (frame->sid >> 3); // TXB0SIDH, top 8 bits of the SID
    tx_buffer[2] = (uint8_t) ((frame->sid & 0b111) << 5); // TXB0SIDL, bottom 3 bits of the SID, according to spec
    tx_buffer[3] = 0; // TXB0EID8
    tx_buffer[4] = 0; // TXB0EID0
    tx_buffer[5] = frame->n_bytes; // TXB0DLC
    for (int i = 0; i < frame->n_bytes; i++) {
        tx_buffer[i + 6] = frame->data[i]; // TXB0D0 ... TXB0D[n_bytes]
    }
    ioctl(dev_file, SPI_IOC_MESSAGE(1), &trx);
    clear_tx_buffer();
    tx_buffer[0] = 0b10000001; // Request to send
    return ioctl(dev_file, SPI_IOC_MESSAGE(1), &trx);
}

struct can_frame* read_can_rx() {
    clear_tx_buffer();
    tx_buffer[0] = 0b10010000;
    ioctl(dev_file, SPI_IOC_MESSAGE(1), &trx);
    struct can_frame* frame = malloc(sizeof(struct can_frame)); // DON'T FORGET TO FREE !!!
    int sid = rx_buffer[1];
    sid = sid << 3;
    sid += (rx_buffer[2] >> 5);
    frame->sid = sid;
    frame->n_bytes = rx_buffer[5];
    frame->data = malloc(8); // FREE THIS TOO !!
    for (int i = 0; i < frame->n_bytes; i++) {
        frame->data[i] = rx_buffer[i + 6];
    }
    return frame;
}

void del_frame(struct can_frame* frame) {
    free(frame->data);
    free(frame);
}

void clear_tx_buffer() {
    for (int i = 0; i < sizeof(tx_buffer); i++) {
        tx_buffer[i] = 0;
    }
}

void print_frame(struct can_frame* frame) {
    printf("SID: %d\nLENGTH:%d\nDATA: [", frame->sid, frame->n_bytes);
    for (int i = 0; i < frame->n_bytes -1; i++) {
        printf("%d, ", frame->data[i]);
    }
    printf("%d]\n", frame->data[frame->n_bytes - 1]);
}

/*
// For testing
int main() {
    initialize();
    print_buffers();
    struct can_frame* f1 = malloc(sizeof(struct can_frame));
    f1->sid = 1234;
    f1->n_bytes = 3;
    f1->data = malloc(8);
    f1->data[0] = 21;
    f1->data[1] = 37;
    f1->data[2] = 69;
    printf("Transmitting frame:\n");
    print_frame(f1);
    transmit_can_frame(f1);
    print_buffers();
    getchar();
    del_frame(f1);
    struct can_frame* f2 = read_can_rx();
    printf("Received frame:\n");
    print_frame(f2);
    print_buffers();
    del_frame(f2);
    deinitialize();
    return 0;
}
*/









