
/*
This file contains functionality enabling a LED to blink in proportion to a given variable (temperature). 

Variables and functions prefixed with private_ should not be accesed from outside this file

Call order:
* 1. LEDBlink_Setup
* 2. LEDBlink_SetRanges
* 3. LEDBlink_SetTemperature

Start Blinking:
* LEDBlink_StartBlinking

Stop Blinking:
* LEDBlink_StopBlinking
* Check in main program with LEDBlink_GetRunning() if thread has stopped before quitting application.

Update Temperature will update the delay after the next blink cycle.

DO NOT CALL private_ FUNCTIONS OUTSIDE THIS FILE. DO NOT ACCESS THE STRUCT OUTSIDE THIS FILE
*/


//Includes
#include <wiringPi.h> //Used for GPIO access.
#include <pthread.h> //Used for creating threads.
//#include <stdio.h>


//Helper struct for bool in C
typedef enum {false, true} bool;



//Forward declarations
void *private_LEDBlink_threadBlink(void*);
int private_LEDBlink_CalculateDelay(void);

 
//This structure holds information about the LEDBlink mechanism
struct LEDBlinkStruct
{
	/* ONLY ACCESS IN LEDBlink.c */
	bool Initialized;
	int Port;
	int Max_Temperature;
	int Min_Temperature;
	int Temperature;
	int Max_Delay;
	int Min_Delay;	
	bool Running;
	pthread_t thread;
	pthread_mutex_t Mutex_Temperature;
	bool WantThreadStop;
	
} private_LEDBlink;


//This function initializes the GPIO ports. Must be called before any other function.
void LEDBlink_Setup(int Port)
{
wiringPiSetup () ;
pinMode(Port,OUTPUT);

pthread_mutex_init(&private_LEDBlink.Mutex_Temperature,NULL);

private_LEDBlink.Port = Port;
private_LEDBlink.WantThreadStop = false;
private_LEDBlink.Initialized = true;


}



//This function will start the blinking of the LED. The LEDBlink Ranges must be set prior.
void LEDBlink_StartBlinking(void)
{
	if(private_LEDBlink.Running) return;
	
	if(!private_LEDBlink.Initialized) return;
	
	pthread_create(&private_LEDBlink.thread,NULL,private_LEDBlink_threadBlink,0);

	
	
}

void LEDBlink_StopBlinking(void)
{
	if(!private_LEDBlink.Running) return;
	
	private_LEDBlink.WantThreadStop = true;
}




/*********SETTERS AND GETTERS**********/
//PORT
void LEDBlink_SetPort(int Port)
{
	private_LEDBlink.Port = Port;
	pinMode(Port,OUTPUT);
}

int LEDBlink_GetPort(void)
{
	return private_LEDBlink.Port;
}
//TEMPERATURE
void LEDBlink_SetTemperature(int Temperature)
{
	pthread_mutex_lock(&private_LEDBlink.Mutex_Temperature);
	int max = private_LEDBlink.Max_Temperature;
	int min = private_LEDBlink.Min_Temperature;
	
	
	if(Temperature > max) Temperature = max;
	if(Temperature < min) Temperature = min;
	
	private_LEDBlink.Temperature = Temperature;
	pthread_mutex_unlock(&private_LEDBlink.Mutex_Temperature);
}
int LEDBlink_GetTemperature(void)
{
	pthread_mutex_lock(&private_LEDBlink.Mutex_Temperature);
	int temp = private_LEDBlink.Temperature;
	pthread_mutex_unlock(&private_LEDBlink.Mutex_Temperature);
	return temp;
	
}
//SET RANGES
void LEDBlink_SetRanges(int min_delay, int max_delay, int min_temp, int max_temp)
{
	private_LEDBlink.Min_Delay = min_delay;
	private_LEDBlink.Max_Delay = max_delay;
	private_LEDBlink.Min_Temperature = min_temp;
	private_LEDBlink.Max_Temperature = max_temp;
}


//RUNNING
bool LEDBlink_GetRunning(void)
{
	return private_LEDBlink.Running;
}

/****PRIVATE FUNCTIONS*********/




//Thread function
void *private_LEDBlink_threadBlink(void* parameter)
{
	private_LEDBlink.Running = true;
	
	int CalculatedDelay = 0;
	
	while(private_LEDBlink.WantThreadStop == false)
	{
		CalculatedDelay = private_LEDBlink_CalculateDelay();
		digitalWrite (private_LEDBlink.Port, HIGH) ;	// On
		delay (CalculatedDelay) ;		
		digitalWrite (private_LEDBlink.Port, LOW) ;	// Off
		delay (CalculatedDelay) ;
		
	}
	
	private_LEDBlink.WantThreadStop = false;
	private_LEDBlink.Running = false;
	
	return NULL;
	
	
}

//The delay the LED should be off/on based on the ranges and temperature.
int private_LEDBlink_CalculateDelay(void)
{
	pthread_mutex_lock(&private_LEDBlink.Mutex_Temperature);
	int Temperature = private_LEDBlink.Temperature;
	pthread_mutex_unlock(&private_LEDBlink.Mutex_Temperature);
	
	int OldMin = private_LEDBlink.Min_Temperature;
	
	int OldRange = private_LEDBlink.Max_Temperature - OldMin;
	
	int NewRange = private_LEDBlink.Min_Delay - private_LEDBlink.Max_Delay;
	
	
	int NewValue = ((((float)Temperature - OldMin) * NewRange ) / OldRange) + private_LEDBlink.Max_Delay;

	int CalculatedDelay = NewValue;
	//printf("Delay: %d \n",CalculatedDelay); //Prints the calculated delay
	return CalculatedDelay;
}
