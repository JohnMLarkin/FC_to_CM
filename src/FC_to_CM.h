#ifndef FC_TO_CM_H
#define FC_TO_CM_H

#include "XBeeAPIParser.h"
#include "mbed.h"
#include "rtos.h"
#include <string> 

#define MAX_FC 6
#define MAX_MSG_LENGTH 110
#define MAX_POD_DATA_BYTES 100

class FC_to_CM {
private:
  XBeeAPIParser _xbee;
  uint32_t _timeout;
  uint64_t _cmAddress;

  //RTOS management
  Thread _rx_thread;

  void _listen_for_rx();                //listens for rx, then runs one of the appropriate responces below, may handle smaller tasks like switching flags 
  void _process_invitation();           //if recieved 0x00, sends 0x10
  void _process_clock_set(char*);
  void _process_clock_test(char*);           //if recieved 0x21, send 0x31 
  void _process_request_data();         //if recieved 0x40, send 0x50

  char flightState;                     //stores the state of the flight from CM broadcasts--enum: preLaunch(0x00),ascent(0x01),descent(0x02),landed(0x03),labTest(0x10)
  char dataInterval;                    //in seconds
  bool goodClock;

  char rsvpState;


public:
  FC_to_CM(PinName tx, PinName rx);
  
};

#endif