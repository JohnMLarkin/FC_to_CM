#include "FC_to_CM.h"

FC_to_CM::FC_to_CM(PinName tx, PinName rx) : _xbee(tx, rx) {
  _timeout = 100;
  _cmAddress = 0x0013A2004192DBBD;
  _rx_thread.start(callback(this, &FC_to_CM::_listen_for_rx));
  flightState = 0x10;
  rsvpState = 0x00;
}

void FC_to_CM::_listen_for_rx() {
  int len;
  char msg[MAX_MSG_LENGTH];
  uint64_t sender = 0;
  _xbee.set_frame_alert_thread_id(osThreadGetId());
  while (true) {
    osSignalWait(0x01,osWaitForever);
    if (_xbee.readable()) {
      len = _xbee.rxPacket(msg, &sender);
      if (len > 0) {
        switch (msg[0]) {
          case 0x00:  // Invitation to connect
            _process_invitation();    //send 0x10 responce
            break;
          case 0x01:  // primed for launch
            flightState = 0x00;   
            break;
          case 0x02:  // launch detected          //flight state 0x02-descent not yet implemented
            flightState = 0x01;
            break;
          case 0x03:  // landing confirmed
            flightState = 0x03;
            break;
          case 0x20: // set clock
            _process_clock_set(msg);
            break;
          case 0x21: // test clock
            _process_clock_test(msg);
            break;
          case 0x40: // request data
            _process_request_data();
            break;
          default:
            // Nothing should fall into this category
            printf("Error! Unexpect rx msg code %0X\r\n", msg[0]);
        }
      }
    }
  }
}
 
void FC_to_CM::_process_invitation() {    //sends 0x10 trasmission to confirm invitation, and responce (decline, clock only, clock and data)
  char msg[2];
  msg[0] = 0x10;
  msg[1] = rsvpState;
  _xbee.txAddressed(_cmAddress, msg, 2);
}

void FC_to_CM::_process_clock_set(char* timeBytes) {
  uint32_t t = 0;
  for (int i = 0; i < 4; i++){            //convert 4 bytes into time_t type
    t |= timeBytes[i+1] << (8 * i);
  }
  set_time(time_t(t));                    //set mbed clock to UNIX time
}

void FC_to_CM::_process_clock_test(char* timeBytes) {
  char msg[2];
  msg[0] = 0x31;
  uint32_t t;
  for (int i = 0; i < 4; i++){            //convert 4 bytes into time_t type
    t |= timeBytes[i+1] << (8 * i);
  }
  if (t != time(NULL)) {                  //compare local time to CM time
    msg[1] = 0x00;  //fail
  } else {
    msg[1] = 0x01;  //pass
  }
  _xbee.txAddressed(_cmAddress, msg, 2);
}
void FC_to_CM::_process_request_data(){
  //send some data  
}