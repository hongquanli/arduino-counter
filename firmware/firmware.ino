#include "pwm_lib.h"
/***************************************************************************************************/
/********************************************* Parameters ******************************************/
/***************************************************************************************************/
static const bool USE_SERIAL_MONITOR = false; // for debug
static const int MSG_LENGTH = 100*4; // transmit 100 time points at a time

byte buffer_tx[MSG_LENGTH];
volatile int buffer_tx_ptr;

volatile bool flag_log_data = false;

static const bool pin_measure = 2;
volatile uint32_t timestamp = 0; // in number of TIMER_PERIOD_us

/***************************************************************************************************/
/******************************************* setup *************************************************/
/***************************************************************************************************/
void setup() 
{

  // Initialize Native USB port
  SerialUSB.begin(2000000);
  while (!SerialUSB);           // Wait until connection is established

  pinMode(pin_measure,INPUT);
  delayMicroseconds(500000);
  
  attachInterrupt(digitalPinToInterrupt(pin_measure), on_rising_edge, RISING);
}

/***************************************************************************************************/
/******************************** timer interrupt handling routine *********************************/
/***************************************************************************************************/
void on_rising_edge()
{
  timestamp = micros();
  flag_log_data = true;
}

/***************************************************************************************************/
/********************************************  main loop *******************************************/
/***************************************************************************************************/
void loop()
{
  
  if (flag_log_data)
  {
    flag_log_data = false;
    
    // field 1: time
    buffer_tx[buffer_tx_ptr++] = byte(timestamp >> 24);
    buffer_tx[buffer_tx_ptr++] = byte(timestamp >> 16);
    buffer_tx[buffer_tx_ptr++] = byte(timestamp >> 8);
    buffer_tx[buffer_tx_ptr++] = byte(timestamp %256);
    
    // send the data after buffer fills to MSG_LENGTH
    if (buffer_tx_ptr == MSG_LENGTH)
    {
      buffer_tx_ptr = 0;
      if(USE_SERIAL_MONITOR)
        SerialUSB.println(timestamp);
      else
        SerialUSB.write(buffer_tx, MSG_LENGTH);
    }
  }
  
}

/***************************************************************************************************/
/*********************************************  utils  *********************************************/
/***************************************************************************************************/
static inline int sgn(int val) 
{
  if (val < 0) return -1;
  if (val == 0) return 0;
  return 1;
}

long signed2NBytesUnsigned(long signedLong, int N)
{
  long NBytesUnsigned = signedLong + pow(256L, N) / 2;
  return NBytesUnsigned;
}
