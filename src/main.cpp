#include <mbed.h>
#include <rtos.h>
#include <XBeeAPIParser.h>
#include "FC_to_CM.h"

DigitalOut led(LED4);
DigitalOut button(p21);
Serial pc(USBTX,USBRX);
FC_to_CM radio(p28,p27);


int main() {
  int transmitSize = 4;
  radio.setResponseClockAndData();

  radio.setDataTransmitSize(transmitSize);

  radio.saveInt(17);
  radio.saveFloat(23.4);

  while(1) {
    led=!led;
    wait(0.5);
    if (button) {
      for (int i = 0; i < transmitSize; i++) {
        pc.printf("%d: Full: %02X Partial: %02X\r\n", i, radio.getFullData(i), radio.getPartialData(i));
      }
    }
  }
}