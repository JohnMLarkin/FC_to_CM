#include "FC_to_CM.h

FC_to_CM::FC_to_CM() {
    
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
          case 0x10:  // Response to invite
            _process_rsvp(sender, msg[1]);
            break;
          case 0x31:  // Response to clock test
            _process_clock_test(sender, msg[1]);
            break;
          case 0x50: // Incoming data
            _process_pod_data(sender, msg, len);
            break;
          default:
            // Nothing should fall into this category
            printf("Error! Unexpect rx msg code %0X\r\n", msg[0]);
        }
      }
    }
  }
}