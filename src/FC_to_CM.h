#ifndef FC_TO_CM_H
#define FC_TO_CM_H

class FC_to_CM {
private:
  XBeeAPIParser _xbee;
  uint32_t _timeout;

  void _listen_for_rx();                //listens for rx, then runs one of the appropriate responces below, may handle smaller tasks like switching flags 
  void _process_invitation();           //if recieved 0x00, sends 0x10
  void _process_clock_set();
  void _process_test_clock();           //if recieved 0x21, send 0x31 
  void _process_request_data();         //if recieved 0x40, send 0x50

  bool primedForLaunch;
  //int dataInterval;                     //in seconds
  bool launchDetected;
  bool landingConfirmed;

public:


};

#endif