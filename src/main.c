#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#include "sensirion_arch_config.h"
#include "sensirion_common.h"
#include "sensirion_i2c.h"

#include "sps30.h"

/**
 * TO USE CONSOLE OUTPUT (printf) PLEASE ADAPT TO YOUR PLATFORM:
 * #define printf(...)
 */

int main(void) {
    
    struct sps30_measurement m;
    int16_t ret;

    /* Initialize I2C bus */
    sensirion_i2c_init();

    /* Busy loop for initialization, because the main loop does not work without
     * a sensor.
     */
    while (sps30_probe() != 0) {
        printk("SPS sensor probing failed\n");
        sensirion_sleep_usec(1000000); /* wait 1s */
    }
    printf("SPS sensor probing successful\n");

    uint8_t fw_major;
    uint8_t fw_minor;
    ret = sps30_read_firmware_version(&fw_major, &fw_minor);
    if (ret) {
        printk("error reading firmware version\n");
    } else {
        printk("FW: %u.%u\n", fw_major, fw_minor);
    }

    char serial_number[SPS30_MAX_SERIAL_LEN];
    ret = sps30_get_serial(serial_number);
    if (ret) {
        printk("error reading serial number\n");
    } else {
        printk("Serial Number: %s\n", serial_number);
    }

    ret = sps30_start_measurement();
    if (ret < 0)
        printk("error starting measurement\n");
    printk("measurements started\n");

    while (1) {
        sensirion_sleep_usec(SPS30_MEASUREMENT_DURATION_USEC); /* wait 1s */
        ret = sps30_read_measurement(&m);
        if (ret < 0) {
            printk("error reading measurement\n");

        } else {

            printk("measured values:\n"
                   "%f pm1.0\n"
                   "%f pm2.5\n"
                   "%f pm4.0\n"
                   "%f pm10.0\n"
                   "%f nc0.5\n"
                   "%f nc1.0\n"
                   "%f nc2.5\n"
                   "%f nc4.5\n"
                   "%f nc10.0\n"
                   "%f typical particle size\n\n",
                   m.mc_1p0, m.mc_2p5, m.mc_4p0, m.mc_10p0, m.nc_0p5, m.nc_1p0,
                   m.nc_2p5, m.nc_4p0, m.nc_10p0, m.typical_particle_size);
    }
}

    return 0;
}
