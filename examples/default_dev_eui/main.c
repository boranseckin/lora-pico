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

#include "gpio.h"
#include "hardware/gpio.h"
#include "pico/board-config.h"
#include "pico/lorawan.h"
#include "pico/stdlib.h"

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

static char event_str[128];
void gpio_event_string(char *buf, uint32_t events);

void gpio_callback(uint gpio, uint32_t events) {
  // Put the GPIO event(s) that just happened into event_str so we can print it
  gpio_event_string(event_str, events);
  printf("GPIO %d %s\n", gpio, event_str);
  uint8_t buffer_Address = 0; 
uint8_t payload_length = 0;
uint8_t data_Buffer[] = {0,0,0,0,0,0,0};

SX126xGetRxBufferStatus(&payload_length,&buffer_Address);

printf("Payload Length: %d\n",payload_length);
printf("Buffer Address: %d\n",buffer_Address);

SX126xReadBuffer(buffer_Address,data_Buffer,payload_length);
printf("Data Buffer: %d %d %d %d %d %d %d\n",data_Buffer[0],data_Buffer[1],data_Buffer[2],data_Buffer[3],data_Buffer[4],data_Buffer[5],data_Buffer[6]);

}

int main(void) {
  char devEui[17];
  RadioError_t error;
  // initialize stdio and wait for USB CDC connect
  stdio_init_all();

  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

  // while (!tud_cdc_connected()) {
  //   tight_loop_contents();
  // }

  sleep_ms(2000);
  gpio_put(PICO_DEFAULT_LED_PIN, 1);

  gpio_set_irq_enabled_with_callback(10, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true,
                                     &gpio_callback);

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
    printf("sanity check failed: %d\n", reg);
    while (1) {
      tight_loop_contents();
    }
  }

  // Setup for RX
  printf("Set Standby\n");
  SX126xClearDeviceErrors();

  error = SX126xGetDeviceErrors();
  printf("error: %d\n", error);
  SX126xSetDio3AsTcxoCtrl(0x07,0xFFF);
  CalibrationParams_t calib_param;
  calib_param.Value = 0xF;
  SX126xCalibrate(calib_param);
  sleep_ms(2000);
  
  SX126xSetStandby(STDBY_RC);
  
  //RadioStatus_t status = SX126xGetStatus();
  //printf("%d %d\n", status.Fields.ChipMode, status.Fields.CmdStatus);
  SX126xSetPacketType(PACKET_TYPE_LORA);
  
  // assert(SX126xGetPacketType() == PACKET_TYPE_LORA);
  SX126xSetRfFrequency(915000000);
  SX126xSetBufferBaseAddress(0x00,0x00); //NOT SURE
  
  ModulationParams_t mod_param;
  mod_param.PacketType = PACKET_TYPE_LORA;
  mod_param.Params.LoRa.SpreadingFactor = LORA_SF7;
  mod_param.Params.LoRa.Bandwidth = LORA_BW_250;
  mod_param.Params.LoRa.CodingRate = LORA_CR_4_5;
  mod_param.Params.LoRa.LowDatarateOptimize = 0;
  
SX126xSetModulationParams(&mod_param);

PacketParams_t packet_param;
        packet_param.PacketType = PACKET_TYPE_LORA;
        packet_param.Params.LoRa.PreambleLength = 0xc;
        packet_param.Params.LoRa.HeaderType = LORA_PACKET_FIXED_LENGTH; //VARIABLE on TX
        packet_param.Params.LoRa.PayloadLength = 0x6;
        packet_param.Params.LoRa.CrcMode = LORA_CRC_OFF;
        packet_param.Params.LoRa.InvertIQ = LORA_IQ_NORMAL;

SX126xSetPacketParams(&packet_param);

SX126xSetDioIrqParams(0x1F7,0x1F7,0x0,0x0);

//SX126xWriteRegister(0x740,0x34);

//SX126xWriteRegister(0x741,0x44);

SX126xSetRx(0xFFFFFF); //Continuous
RadioStatus_t status = SX126xGetStatus();
printf("%d %d\n", status.Fields.ChipMode, status.Fields.CmdStatus);


  // SX126xSetPaConfig(0x04, 0x07, 0x00, 0x01);
  // SX126xSetTxParams(0x16, 0x02);
  //RadioStatus_t status = SX126xGetStatus();
  //printf("%d %d\n", status.Fields.ChipMode, status.Fields.CmdStatus);

  printf("BYE\n");

  // do nothing
  while (1) {
    tight_loop_contents();
  }

  return 0;
}

static const char *gpio_irq_str[] = {
    "LEVEL_LOW",  // 0x1
    "LEVEL_HIGH", // 0x2
    "EDGE_FALL",  // 0x4
    "EDGE_RISE"   // 0x8
};

void gpio_event_string(char *buf, uint32_t events) {
  for (uint i = 0; i < 4; i++) {
    uint mask = (1 << i);
    if (events & mask) {
      // Copy this event string into the user string
      const char *event_str = gpio_irq_str[i];
      while (*event_str != '\0') {
        *buf++ = *event_str++;
      }
      events &= ~mask;

      // If more events add ", "
      if (events) {
        *buf++ = ',';
        *buf++ = ' ';
      }
    }
  }
  *buf++ = '\0';
}