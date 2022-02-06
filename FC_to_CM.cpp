#include "FC_to_CM.h"

FC_to_CM::FC_to_CM(PinName tx, PinName rx) {
  _xbee = new XBeeAPIParser(tx, rx);
  _timeout = 100ms;
  _rx_thread.start(callback(this, &FC_to_CM::_listen_for_rx));    //start rx thread to read in serial communication from xbee modem
  _flightState = 0x00;
  _rsvpState = 0x00;
  _dataTransmitSize = 0;
  _partialDataIndex = 0;
  _readyToSendData = false;
}

void FC_to_CM::_listen_for_rx() {
  int len;
  char msg[MAX_MSG_LENGTH];
  uint64_t sender = 0;
  _xbee->set_frame_alert_thread_id(ThisThread::get_id());
  while (true) {
    osSignalWait(0x01,osWaitForever);
    if (_xbee->readable()) {
      len = _xbee->rxPacket(msg, &sender);
      if (len > 0) {
        switch (msg[0]) {
          case 0x00:  // Invitation to connect
            _process_invitation(sender);    //send 0x10 response
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
            printf("Error! Unexpected rx msg code %0X\n", msg[0]);
        }
      }
    }
  }
}
 
void FC_to_CM::_process_invitation(uint64_t address) {    //sends 0x10 transmission to confirm invitation, and response (decline, clock only, clock and data)
  char msg[2];
  msg[0] = 0x10;
  msg[1] = _rsvpState;
  _xbee->txAddressed(address, msg, 2);
  printf("I've got an invitation!\n");
}

void FC_to_CM::_process_clock_set(char* timeBytes) {
  time_t t = 0;
  for (int i = 0; i < 4; i++){            //convert 4 bytes into time_t type
    t = (t<<8) | timeBytes[i+1];
  }
  set_time(t);                    //set mbed clock to UNIX time
  printf("My clock has been set!\n");
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
  _xbee->txAddressed(address, msg, 2);
  printf("My clock has been tested! any errors?: %d\n", msg[1]);
}


void FC_to_CM::_process_request_data( uint64_t address){
  if (_readyToSendData) {                                     //check to see if data set is ready to be transmitted
    if(_data_mutex.trylock_for(_timeout)){                    //lockout, so transfer data cannot be called and change data durring transmission
      _readyToSendData = false;                               //clear flag
      _partialDataIndex = 0;
      char msg[MAX_MSG_LENGTH];                               //create message
      msg[0] = 0x50;                                          //sending data is code 0x50
      for (int i = 0; i < _dataTransmitSize; i++) {           //load complete data set into message
        msg[i + 1] = _fullDataSet[i];
      }
    _xbee->txAddressed(address, msg, _dataTransmitSize + 1);   //send msg
      _data_mutex.unlock();                                   //unlock mutex
    } 
  } else {                                                    //if data is not ready
    char msg[1] = {0x50};
    _xbee->txAddressed(address, msg, 1);                       //send empty data packet
  }
  printf("data has been requested of me!\r\n");
}

void FC_to_CM::_transferPartialData() {                       //moves data in partial data to full data when ready to transmit
  if (_data_mutex.trylock_for(_timeout)) {                    //lockout
    for (int i = 0; i < _dataTransmitSize; i++) {
      _fullDataSet[i] = _partialDataSet[i];
    }
    _readyToSendData = true;
    _partialDataIndex = 0;
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

template<typename T>
void FC_to_CM::_addBytesToData(T value) {
  int size = sizeof(T);
  if ((size + _partialDataIndex) <= _dataTransmitSize) {       //check to see if room is availible in data set
    unsigned char bytes[16];                       //create array of bytes (max size is 16 bytes)

    memcpy(bytes, &value, size);                     //copy value's bytes into array

    for (int i = 0; i < size; i++) {
      _partialDataSet[_partialDataIndex++] = bytes[i];    //add bytes to partial data
    } 
  }
  else {
    printf("data could not fit in storage\r\n");              //failed, not enough room
    
  }
  if (_partialDataIndex == _dataTransmitSize) {               //once enough data has been added, transfer partial data to full data
    _transferPartialData();
  }
}