#include <mbed.h>
#include <rtos.h>
#include <XBeeAPIParser.h>
#include "FC_to_CM.h"

DigitalOut led(LED4);
DigitalOut button(p21);
Serial pc(USBTX,USBRX);
FC_to_CM radio(p28,p27);


int main() {
  time_t thisTime;
  radio.setResponceClockAndData();

  radio.setDataTranmitSize(4);

  radio.saveInt(17);

  while(1) {
    led=!led;
    wait(0.5);
    if (button) {
      thisTime = time(NULL);
      pc.printf("%d %s\r\n", radio.getFlightState(), ctime(&thisTime));
    }
  }
}