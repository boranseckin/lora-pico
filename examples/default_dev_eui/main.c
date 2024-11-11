/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 *
 * This example displays devices default LoRaWAN Dev EUI which is based
 * on the Pico SDK's pico_get_unique_board_id(...) API which uses the
 * on board NOR flash device 64-bit unique ID.
 *
 * https://raspberrypi.github.io/pico-sdk-doxygen/group__pico__unique__id.html
 *
 */

#include <stdio.h>
#include <string.h>

#include "pico/board-config.h"
#include "pico/lorawan.h"
#include "pico/stdlib.h"
#include "tusb.h"

#include "board.h"
#include "eeprom-board.h"
#include "rtc-board.h"
#include "spi.h"
#include "sx126x-board.h"

const struct lorawan_sx126x_settings sx126x_settings = {.spi = {.inst = spi0,
                                                                .mosi = PICO_DEFAULT_SPI_TX_PIN,
                                                                .miso = PICO_DEFAULT_SPI_RX_PIN,
                                                                .sck = PICO_DEFAULT_SPI_SCK_PIN,
                                                                .nss = RADIO_NSS},
                                                        .reset = RADIO_RESET,
                                                        .dio1 = RADIO_DIO_1};

int main(void) {
  char devEui[17];

  // initialize stdio and wait for USB CDC connect
  stdio_init_all();

  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

  // while (!tud_cdc_connected()) {
  //   tight_loop_contents();
  // }

  sleep_ms(2000);
  gpio_put(PICO_DEFAULT_LED_PIN, 1);

  // get the default Dev EUI as a string and print it out
  printf("HELLO\n");
  printf("Pico LoRaWAN - Default Dev EUI = %s\n", lorawan_default_dev_eui(devEui));

  EepromMcuInit();

  RtcInit();
  SpiInit(&SX126x.Spi, (SpiId_t)((sx126x_settings.spi.inst == spi0) ? 0 : 1),
          sx126x_settings.spi.mosi, sx126x_settings.spi.miso, sx126x_settings.spi.sck, NC);

  SX126x.Spi.Nss.pin = sx126x_settings.spi.nss;
  SX126x.Reset.pin = sx126x_settings.reset;
  SX126x.DIO1.pin = sx126x_settings.dio1;

  SX126xIoInit();

  sleep_ms(2000);
  SX126xReset();
  sleep_ms(2000);

  // Sanity Check
  uint8_t reg = SX126xReadRegister(0x0740);
  if (reg == 0x14) {
    printf("sanity check passed\n");
  } else {
    printf("sanity check failed\n");
    while (1) {
      tight_loop_contents();
    }
  }

  SX126xSetStandby(STDBY_RC);
  SX126xSetPacketType(PACKET_TYPE_LORA);
  assert(SX126xGetPacketType() == PACKET_TYPE_LORA);
  // SX126xSetRfFrequency(915000000);
  // SX126xSetPaConfig(0x04, 0x07, 0x00, 0x01);
  // SX126xSetTxParams(0x16, 0x02);

  RadioStatus_t status = SX126xGetStatus();
  printf("%d %d\n", status.Fields.ChipMode, status.Fields.CmdStatus);

  printf("BYE\n");

  // do nothing
  while (1) {
    tight_loop_contents();
  }

  return 0;
}
