/**
 * @file main.cpp
 * @author Volodymyr Shymanskyy
 * @license This project is released under the MIT License (MIT)
 * @copyright Copyright (c) 2015 Volodymyr Shymanskyy
 * @date Mar 2015
 * @brief
 */


///NB pins use BCM instead of wiring pi layout for some reason \('-')/-
 //#define BLYNK_DEBUG

#define BLYNK_PRINT stdout


#ifdef RASPBERRY
  #include <BlynkApiWiringPi.h>
#else
  #include <BlynkApiLinux.h>
#endif

#include <BlynkSocket.h>
#include <BlynkOptionsParser.h>
static BlynkTransportSocket _blynkTransport;
BlynkSocket Blynk(_blynkTransport);
static const char *auth, *serv;
static uint16_t port;

#include <BlynkWidgets.h>

//INCLUDES
#include <wiringPi.h>
#include <stdio.h> //For printf functions
#include <stdlib.h> // For system functions
#include <signal.h>//for keyboard interrupt
#include <math.h>
#include <wiringPiSPI.h>

//FUNCTION DECLARATIONS
//sensor functions/GPIO
float readADC(int);
void toggleDelay(void);
void initPorts();
float getTemp();
float getHumid();
float getLight();
float round(float,int);

//time functions
void getCurrentTime(void);
int getHours(void);
int getMins(void);
int getSecs(void);


//GLOBAL VARIABLES
float temp;
float humidity;
float light;
int HH,MM,SS;//time
int delayArr[]={1,2,5};
int delayIndex = 0;
//debouncing
long lastInterruptTime = 0;
long debounce = 200;

BlynkTimer tmr;

BLYNK_WRITE(V1) {
    printf("Got a value: %s\n", param[0].asStr());
}




void setup() {
    Blynk.begin(auth, serv, port);
    //tmr.setInterval(1000, [](){
    //Blynk.virtualWrite(V0, BlynkMillis()/1000);
    //});
}


void loop() {
    Blynk.run();
    
//tmr.run();
}

int main(int argc, char* argv[]) {
    parse_options(argc, argv, auth, serv, port);
    setup();
    initPorts();
    while(true) {
        loop();
	temp = getTemp();
	temp = round(temp,0);
	Blynk.virtualWrite(V4,temp);
	humidity = getHumid();
	Blynk.virtualWrite(V5,humidity);
	light = getLight();
	Blynk.virtualWrite(V6,light);

	int hours = getHours();
	int mins = getMins();
	int secs = getSecs();

	printf("Time:%02d:%02d:%02d\t",hours,mins,secs);
	printf("Temp:%.0fC\tHumid:%.1fV\tLight:%.0f\n",temp,humidity,light);
	Blynk.virtualWrite(V0,hours,":",mins,":",secs,"\tT:",(int)temp,"C \tH:",humidity," V\tLight:",(int)light,"\n");    

	//delay for display of next value
	int temporary = getSecs();
	temporary = temporary+delayArr[delayIndex];//tempor+delay delay=(1,5,10)
	if(temporary>59){
	temporary = temporary-60;
	}
	while(secs!=temporary){//while seconds are unchanged //do nothing
	secs = getSecs();
	delay(200);//less computationally heavy... delay not so high as to potentiall skip over a seconds
	}
	}
    return 0;
}


void initPorts(void){
//analog in for voltage divider 0V-3.3V translates to
//set port 4 to input analog
wiringPiSetup();
pinMode(26,INPUT);
pullUpDnControl (26,PUD_UP);
int x=wiringPiISR(26,INT_EDGE_FALLING,toggleDelay);
if(x==-1){printf("Something wrong");}
int z =wiringPiSPISetup(0, 1350000);//sets up spi channel 0 and sets freq=1.35MHz
if(z==-1){printf("Something wrong");}
}

float readADC(int channel){//channel specifies the analogue input
//SPI communication
//The ADC takes 3, 8-bit values as an input
unsigned char bufferADC[3];

bufferADC[0]=1;//start bit with 7 leading 0s (0b00000001)
bufferADC[1]=((0b1000+channel)<<4);//config bits, 0b1000=single ended communication(not differential)
bufferADC[2]= 0;//ADC doesnt care what these vals are
wiringPiSPIDataRW(0,bufferADC,3);//chan=0 (SPI channel=/=ADC channel)

float reading = ((bufferADC[1] & 0b11) <<8 ) + bufferADC[2];
return reading;//returns a value of 0-1023 (10-bit value)
}

float getTemp(){
float temp = readADC(0);//0-1023
temp = (temp/1023)*3.3;//puts value into Volts
temp = (temp-0.5)/0.01;//equation for ambient temp, Ta = (Vout-V0*)/Tc
return temp;
}

float getHumid(){
float humid = readADC(1);//0-1023, channel 1
humid = (humid/1023)*3.3;//voltage
return humid;
}

float getLight(){//function could have been accomplished just by calling readADC(2) but for the same of congruenty it was added
float light = readADC(2);
return light;
}

float round(float var,int decplaces){
    float value = (int)(var *(pow(10,decplaces)) + 0.5);
    return (float)value / (pow(10,decplaces));
}


void getCurrentTime(void){
  time_t rawtime;
  struct tm * timeinfo;
  time ( &rawtime );
  timeinfo = localtime ( &rawtime );

  HH = timeinfo ->tm_hour;
  MM = timeinfo ->tm_min;
  SS = timeinfo ->tm_sec;
}

int getHours(void){
    getCurrentTime();
    return HH;
}

int getMins(void){
	getCurrentTime();
    return MM;
}

int getSecs(void){
	getCurrentTime();
    return SS;
}


void toggleDelay(void){
long interruptTime = millis();
	if (interruptTime - lastInterruptTime> debounce){
	delayIndex++;
		if(delayIndex==3){
		delayIndex=0;
		}
	}
            lastInterruptTime = interruptTime;
}
