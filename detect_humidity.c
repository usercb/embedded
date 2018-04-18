/*
 *      dht22.c:
 *	Simple test program to test the wiringPi functions
 *	Based on the existing dht11.c
 *	Amended by technion@lolware.net
 */

#include <wiringPi.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#include <errno.h>
#include <softPwm.h>
#include <signal.h>
//#include "locking.h"


#define MAXTIMINGS 85
#define MOTORCONTROL 23
#define RGBLEDPOWER  24 //BCM_GPIO 19

#define RED	7//27
#define GREEN	9//28
#define BLUE	8//29
#define FAN 22

void sig_handler(int signo); // SIGINT 사용 마지막 종료 함수


int ret_humid, ret_temp;

//static int DHTPIN = 7;
static int DHTPIN = 11;
static int dht22_dat[5] = {0,0,0,0,0};

static uint8_t sizecvt(const int read)
{
  /* digitalRead() and friends from wiringpi are defined as returning a value
  < 256. However, they are returned as int() types. This is a safety function */

  if (read > 255 || read < 0)
  {
    printf("Invalid data from wiringPi library\n");
    exit(EXIT_FAILURE);
  }
  return (uint8_t)read;
}

int read_dht22_dat()
{
  uint8_t laststate = HIGH;
  uint8_t counter = 0;
  uint8_t j = 0, i;

  dht22_dat[0] = dht22_dat[1] = dht22_dat[2] = dht22_dat[3] = dht22_dat[4] = 0;

  // pull pin down for 18 milliseconds
  pinMode(DHTPIN, OUTPUT);
  digitalWrite(DHTPIN, HIGH);
  delay(10);
  digitalWrite(DHTPIN, LOW);
  delay(18);
  // then pull it up for 40 microseconds
  digitalWrite(DHTPIN, HIGH);
  delayMicroseconds(40); 
  // prepare to read the pin
  pinMode(DHTPIN, INPUT);

  // detect change and read data
  for ( i=0; i< MAXTIMINGS; i++) {
    counter = 0;
    while (sizecvt(digitalRead(DHTPIN)) == laststate) {
      counter++;
      delayMicroseconds(1);
      if (counter == 255) {
        break;
      }
    }
    laststate = sizecvt(digitalRead(DHTPIN));

    if (counter == 255) break;

    // ignore first 3 transitions
    if ((i >= 4) && (i%2 == 0)) {
      // shove each bit into the storage bytes
      dht22_dat[j/8] <<= 1;
      if (counter > 50)
        dht22_dat[j/8] |= 1;
      j++;
    }
  }

  // check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
  // print it out if data is good
  if ((j >= 40) && 
      (dht22_dat[4] == ((dht22_dat[0] + dht22_dat[1] + dht22_dat[2] + dht22_dat[3]) & 0xFF)) ) {
        float t, h;
		
        h = (float)dht22_dat[0] * 256 + (float)dht22_dat[1];
        h /= 10;
        t = (float)(dht22_dat[2] & 0x7F)* 256 + (float)dht22_dat[3];
        t /= 10.0;
        if ((dht22_dat[2] & 0x80) != 0)  t *= -1;
		
		ret_humid = (int)h;
		ret_temp = (int)t;
		//printf("Humidity = %.2f %% Temperature = %.2f *C \n", h, t );
		//printf("Humidity = %d Temperature = %d\n", ret_humid, ret_temp);
		
    return ret_humid;
  }
  else
  {
    printf("Data not good, skip\n");
    return 0;
  }
}

int main (void)
{
	signal(SIGINT, (void *)sig_handler);
	int received_humid;

	DHTPIN = 11;

	if (wiringPiSetup() == -1)
		exit(EXIT_FAILURE) ;
	
	if (setuid(getuid()) < 0)
	{
		perror("Dropping privileges failed\n");
		exit(EXIT_FAILURE);
	}


  
	pinMode(RGBLEDPOWER, OUTPUT);
	pinMode(RED, OUTPUT);
	pinMode(MOTORCONTROL, OUTPUT);
	pinMode(FAN, OUTPUT);
	softPwmCreate(MOTORCONTROL, 0, 100);
	

	int i;
	int delaytime = 100;
	digitalWrite(RGBLEDPOWER, 1);
	while(1){

	while (read_dht22_dat() == 0) 
	{
		delay(500); // wait 1sec to refresh
	}
	received_humid = ret_humid;
	printf("Humidity = %d\n", received_humid);
	if (received_humid >= 70)
	{
		digitalWrite(RED,1);
		delay(1000);
		digitalWrite(RED,0);
		digitalWrite (FAN,1);
		softPwmWrite(MOTORCONTROL,10);
		delay(1000);
			}

	}
	return 0;
}

int Bpluspinmodeset(void)
{
	pinMode (MOTORCONTROL, OUTPUT);
}

void sig_handler(int signo)
{
	printf("process stop\n");
	digitalWrite(MOTORCONTROL,0);
	digitalWrite(RED, 0);
	digitalWrite(GREEN, 0);
	digitalWrite(BLUE, 0);
	digitalWrite(RGBLEDPOWER, 0); //Off
	digitalWrite(FAN,0);	
	exit(0);
}
