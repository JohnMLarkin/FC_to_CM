#include "FC_to_CM.h"

FC_to_CM::FC_to_CM(PinName tx, PinName rx) : _xbee(tx, rx) {
  _timeout = 100;
  _rx_thread.start(callback(this, &FC_to_CM::_listen_for_rx));    //start rx thread to read in serial communication from xbee modem
  _flightState = 0x00;
  _rsvpState = 0x00;
  _dataTransmitSize = 0;
  _incDataIndex = 0;
  _readyToSendData = false;
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
            _process_invitation(sender);    //send 0x10 responce
            break;
          case 0x01:  // primed for launch
            _flightState = 0x01;
            _dataInterval = msg[1];
            break;
          case 0x02:  // launch detected          //flight state 0x03-descent not yet implemented
            _flightState = 0x02;
            break;
          case 0x03:  // landing confirmed
            _flightState = 0x04;
            break;
          case 0x20: // set clock
            _process_clock_set(msg);
            break;
          case 0x21: // test clock
            _process_clock_test(msg, sender);
            break;
          case 0x40: // request data
            _process_request_data(sender);
            break;
          default:
            // Nothing should fall into this category
            printf("Error! Unexpect rx msg code %0X\r\n", msg[0]);
        }
      }
    }
  }
}
 
void FC_to_CM::_process_invitation(uint64_t address) {    //sends 0x10 trasmission to confirm invitation, and responce (decline, clock only, clock and data)
  char msg[2];
  msg[0] = 0x10;
  msg[1] = _rsvpState;
  _xbee.txAddressed(address, msg, 2);
  printf("Ive got an inviatation!");
}

void FC_to_CM::_process_clock_set(char* timeBytes) {
  time_t t = 0;
  for (int i = 0; i < 4; i++){            //convert 4 bytes into time_t type
    t = (t<<8) | timeBytes[i+1];
  }
  set_time(t);                    //set mbed clock to UNIX time
  printf("my clock has been set!");
}

void FC_to_CM::_process_clock_test(char* timeBytes, uint64_t address) {
  char msg[2];
  msg[0] = 0x31;
  time_t t = 0;
  for (int i = 0; i < 4; i++){            //convert 4 bytes into time_t type
    t = (t<<8) | timeBytes[i+1];
  }
  time_t now;
  now = time(NULL);
  int deltaT = now - t;
  if ((deltaT >= 1) || (deltaT <= -1)) {                  //compare local time to CM time
    _goodClock = 0;
    msg[1] = 0x01;  //fail
  } else {
    _goodClock = 1;
    msg[1] = 0x00;  //pass
  }
  _xbee.txAddressed(address, msg, 2);
  printf("my clock has been tested! any errors?: %d", msg[1]);
}
void FC_to_CM::_process_request_data( uint64_t address){
  if (_readyToSendData) {
    if(_data_mutex.trylock_for(_timeout)){
      _readyToSendData = false;
      char msg[MAX_MSG_LENGTH];
      msg[0] = 0x50;
      for (int i = 0; i < _dataTransmitSize; i++) {
        msg[i + 1] = _completeDataSet[i];
      }
    _xbee.txAddressed(address, msg, _dataTransmitSize + 1);
      _data_mutex.unlock();
    } 
  } else {
    char msg[1] = {0x50};
    _xbee.txAddressed(address, msg, 1);   //send empty data packet
  }
  printf("data has been requested of me!");
}

template<typename T>
bool FC_to_CM::_addBytesToData(T value) {
  int size = sizeof(T);
  if ((size + _incDataIndex) < _dataTransmitSize) {       //check to see if room is availible in data set
    unsigned char bytes[sizeof(T)];

    memcpy(bytes, &value, sizeof(T));

    for (unsigned int i = 0; i < sizeof(T); i++) {
      bytes[i] = _incompleteDataSet[_incDataIndex++];    //add value as series of bytes
    }
  }
  else {
    return false;   //failed
  }
  if (_incDataIndex == _dataTransmitSize) {
    _transferIncDataToCData();
  }
  return true;    //sucessful
}

void FC_to_CM::_transferIncDataToCData() {
  if (_data_mutex.trylock_for(_timeout)) {
    for (int i = 0; i < _dataTransmitSize; i++) {
      _completeDataSet[i] = _incompleteDataSet[i];
      _incompleteDataSet[i] = 0;
    }
    _readyToSendData = true;
    _data_mutex.unlock();
  }
}

void FC_to_CM::saveInt(int val) {
  _addBytesToData(val);
}

void FC_to_CM::saveFloat(float val) {
  _addBytesToData(val);

}
void FC_to_CM::saveFloatAsInt(float val, int precision) {
  float newVal = val;
  for (int i = 0; i < precision; i++){                      //multiply val by 10^(precision)
    newVal = newVal * 10;
  }
  int intVal = int(newVal);
  _addBytesToData(intVal);
}