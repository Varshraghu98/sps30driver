#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <stdio.h>

#include <zephyr/net/openthread.h>
#include <openthread/thread.h>
#include <openthread/udp.h>

#include "sensirion_arch_config.h"
#include "sensirion_common.h"
#include "sensirion_i2c.h"
#include "sps30.h"

#define SLEEP_TIME_MS 4000

/**
 * TO USE CONSOLE OUTPUT (printf) PLEASE ADAPT TO YOUR PLATFORM:
 * #define printf(...)
 */

int main(void) {
    otError error = OT_ERROR_NONE;
    otInstance *myInstance;
    otUdpSocket mySocket;
    otMessageInfo messageInfo;

    struct sps30_measurement m;
    int16_t ret;
    char buf[512];

    /* Initialize I2C bus */
    sensirion_i2c_init();

    /* Busy loop for initialization, because the main loop does not work without
     * a sensor.
     */
    while (sps30_probe() != 0) {
        printk("SPS sensor probing failed\n");
        sensirion_sleep_usec(1000000); /* wait 1s */
    }
    printk("SPS sensor probing successful\n");

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
    if (ret < 0) {
        printk("error starting measurement\n");
    }
    printk("measurements started\n");

    // OpenThread initialization
    myInstance = openthread_get_default_instance();
    memset(&messageInfo, 0, sizeof(messageInfo));
    otIp6AddressFromString("ff03::1", &messageInfo.mPeerAddr);
    messageInfo.mPeerPort = 9876;

    error = otUdpOpen(myInstance, &mySocket, NULL, NULL);
    if (error != OT_ERROR_NONE) {
        printk("Failed to open UDP socket: %d\n", error);
        return -1;
    }

    while (1) {
        sensirion_sleep_usec(SPS30_MEASUREMENT_DURATION_USEC); /* wait 1s */
        ret = sps30_read_measurement(&m);
        if (ret < 0) {
            printk("error reading measurement\n");
        } else {
            printk("measured values:\n"
                   "%.3f pm1.0\n"
                   "%.3f pm2.5\n"
                   "%.3f pm4.0\n"
                   "%.3f pm10.0\n"
                   "%.3f nc0.5\n"
                   "%.3f nc1.0\n"
                   "%.3f nc2.5\n"
                   "%.3f nc10.0\n"
                   "%.3f typical particle size\n\n",
                   m.mc_1p0, m.mc_2p5, m.mc_4p0, m.mc_10p0, m.nc_0p5, m.nc_1p0,
                   m.nc_2p5, m.nc_10p0, m.typical_particle_size);
            


             // Format the sensor data as a string
            int snprintf_ret = snprintf(buf, sizeof(buf),
                                        "pm1.0: %.3f, pm2.5: %.3f, pm4.0: %.3f, pm10.0: %.3f, nc0.5: %.3f, nc1.0: %.3f, nc2.5: %.3f, nc10.0: %.3f, typical size: %.3f",
                                        m.mc_1p0, m.mc_2p5, m.mc_4p0, m.mc_10p0,
                                        m.nc_0p5, m.nc_1p0, m.nc_2p5, m.nc_10p0, m.typical_particle_size);
            // Check if snprintf was successful
            if (snprintf_ret < 0 || snprintf_ret >= sizeof(buf)) {
                printk("Failed to format data or buffer too small\n");
                continue;
            }
            // Debug print the buffer content
        printk("Formatted data to send: %s\n", buf);

            otMessage *test_Message = otUdpNewMessage(myInstance, NULL);
            error = otMessageAppend(test_Message, buf, (uint16_t)strlen(buf));
            if (error == OT_ERROR_NONE) {
                error = otUdpSend(myInstance, &mySocket, test_Message, &messageInfo);
                if (error == OT_ERROR_NONE) {
                    printk("UDP message sent successfully.\n");
                } else {
                    printk("Failed to send UDP message: %d\n", error);
                }
            } else {
                printk("Failed to append message: %d\n", error);
                otMessageFree(test_Message);
            }
        }
    }

    error = otUdpClose(myInstance, &mySocket);
    if (error != OT_ERROR_NONE) {
        printk("Failed to close UDP socket: %d\n", error);
    }

    return 0;
}
