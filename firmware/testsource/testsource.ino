static const int pin_source = 2;

void setup() 
{
  pinMode(pin_source,OUTPUT);
  delayMicroseconds(500000);
}

/***************************************************************************************************/
/********************************************  main loop *******************************************/
/***************************************************************************************************/
void loop()
{
  digitalWrite(pin_source,HIGH);
  delayMicroseconds(4050);
  digitalWrite(pin_source,LOW);
  delayMicroseconds(4050);
}
