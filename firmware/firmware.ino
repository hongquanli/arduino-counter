#include "pwm_lib.h"
#include <DueTimer.h>

#define MUX_ADDR 0x70 //7-bit unshifted default I2C Address

/***************************************************************************************************/
/********************************************* Parameters ******************************************/
/***************************************************************************************************/
static const float TIMER_PERIOD_us = 5000; // in us
static const bool USE_SERIAL_MONITOR = false; // for debug
static const int MSG_LENGTH = 50*10;

static const int CMD_LENGTH = 4;
byte buffer_rx[500];
byte buffer_tx[MSG_LENGTH];
volatile int buffer_rx_ptr;
volatile int buffer_tx_ptr;

// full scale values
static const float flow_FS = 200;
static const float paw_FS = 100;

// measured variables
float mflow_1 = 0;
float mpaw = 0;

volatile bool flag_log_data = false;
volatile bool flag_read_sensor = false;

// data logging
# define LOGGING_UNDERSAMPLING  1
volatile int counter_log_data = 0;

// other variables
uint16_t tmp_uint16;
int16_t tmp_int16;
long tmp_long;
volatile uint32_t timestamp = 0; // in number of TIMER_PERIOD_us

/***************************************************************************************************/
/********************************************* sensors *********************************************/
/***************************************************************************************************/
uint8_t cmd[1];

uint16_t ch1;
uint16_t ch2;
uint16_t ch3;

/***************************************************************************************************/
/******************************************* setup *************************************************/
/***************************************************************************************************/
void setup() 
{

  // Initialize Native USB port
  SerialUSB.begin(2000000);
  while (!SerialUSB);           // Wait until connection is established
  buffer_rx_ptr = 0;

  analogReadResolution(12);
  delayMicroseconds(500000);


  // start the timer
  Timer3.attachInterrupt(timer_interruptHandler);
  Timer3.start(TIMER_PERIOD_us);

}

/***************************************************************************************************/
/******************************** timer interrupt handling routine *********************************/
/***************************************************************************************************/
void timer_interruptHandler()
{
  timestamp = timestamp + 1;

  // read sensor value
  flag_read_sensor = true;
  
  // send data to host computer
  counter_log_data = counter_log_data + 1;
  if (counter_log_data >= LOGGING_UNDERSAMPLING)
  {
    counter_log_data = 0;
    flag_log_data = true;
  }
}

/***************************************************************************************************/
/********************************************  main loop *******************************************/
/***************************************************************************************************/
void loop()
{

  if (flag_read_sensor)
  {
    ch1 = analogRead(A1);
    ch2 = analogRead(A2);
    ch3 = analogRead(A3);
    flag_read_sensor = false;
  }

  if (flag_log_data)
  {
    flag_log_data = false;
    
    // field 1: time
    buffer_tx[buffer_tx_ptr++] = byte(timestamp >> 24);
    buffer_tx[buffer_tx_ptr++] = byte(timestamp >> 16);
    buffer_tx[buffer_tx_ptr++] = byte(timestamp >> 8);
    buffer_tx[buffer_tx_ptr++] = byte(timestamp %256);

    // field 2 ch1
    buffer_tx[buffer_tx_ptr++] = byte(ch1 >> 8);
    buffer_tx[buffer_tx_ptr++] = byte(ch1 % 256);

    // field 3 ch2
    buffer_tx[buffer_tx_ptr++] = byte(ch2 >> 8);
    buffer_tx[buffer_tx_ptr++] = byte(ch2 % 256);

    // field 4 ch3
    buffer_tx[buffer_tx_ptr++] = byte(ch3 >> 8);
    buffer_tx[buffer_tx_ptr++] = byte(ch3 % 256);

    if (buffer_tx_ptr == MSG_LENGTH)
    {
      buffer_tx_ptr = 0;
      if(USE_SERIAL_MONITOR)
      {
        SerialUSB.println(mflow_1);
      }
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
