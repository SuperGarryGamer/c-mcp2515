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

uint8_t tx_buffer[14]; // Instruction byte + all the necessary MCP2515 registers
uint8_t rx_buffer[14];
struct spi_ioc_transfer trx;
int dev_file;
int dev_response;

int initialize() {
    trx.rx_buf = rx_buffer;
    trx.tx_buf = tx_buffer;
    trx.len = 13;
    trx.speed_hz = SPEED;
    trx.delay_usecs = 0;
    trx.bits_per_word = 0;

    dev_file = open(SPI_DEVICE, O_RDWR);
    if (dev_file != 0) {
        printf("Could not open SPI device\n");
        return -1;
    }

    dev_response = ioctl(dev_file, SPI_IOC_WR_MODE32, 0);
    if (dev_response != 0) {
        printf("Could not set SPI mode\n");
        close(dev_file);
        return -1;
    }

    dev_response = ioctl(dev_file, SPI_IOC_WR_MAX_SPEED_HZ, SPEED);
    if (dev_response != 0) {
        printf("Could not set SPI speed\n");
        close(dev_file);
        return -1;
    }
    
    for (int i = 0; i < 13; i++) {
        printf("%d", tx_buffer[i]);
    }
    printf("Resetting MCP2515\n");
    reset();
    for (int i = 0; i < 13; i++) {
        printf("%d", tx_buffer[i]);
    }
}

int reset() {
    tx_buffer[0] = 0b11000000; // RESET instruction
    dev_response = ioctl(dev_file, SPI_IOC_MESSAGE(1), &trx);
    return 0;
}

int deinitialize() {
    close(dev_file);
    return 0;
}

int main() {

    return 0;
}