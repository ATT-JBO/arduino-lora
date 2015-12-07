/****
 *  AllThingsTalk Developer Cloud IoT experiment for LoRa
 *  version 1.0 dd 09/11/2015
 *  Original author: Jan Bogaerts 2015
 *
 *  This sketch is part of the AllThingsTalk LoRa rapid development kit
 *  -> http://www.allthingstalk.com/lora-rapid-development-kit
 *
 *  This example sketch is based on the Proxilmus IoT network in Belgium
 *  The sketch and libs included support the
 *  - MicroChip RN2483 LoRa module
 *  - Embit LoRa modem EMB-LR1272
 *  
 *  For more information, please check our documentation
 *  -> http://docs.smartliving.io/kits/lora
 *
 * EXPLENATION:
 * Each time the door opens a counter is incremented locally on your LoRa device.
 * Every 30 seconds, if the count has changed, it will be sent to your SmartLiving account.
 * As soon as a count of 20 is reached, a notification is sent out to remind you that cleaning is in order.
 * A pushbutton on the device allows you to reset the count when cleaning is done.
 * This can also be seen as validation that the cleaning crew has actually visited the facility.

 */

#include <Wire.h>
#include <ATT_LoRa_IOT.h>
#include "keys.h"
#include <MicrochipLoRaModem.h>

#define SERIAL_BAUD 57600

int pushButton = 20;          
int doorSensor = 4;
MicrochipLoRaModem Modem(&Serial1);
ATTDevice Device(&Modem);

#define SEND_MAX_EVERY 30000                                //the mimimum time between 2 consecutive updates of visit counts that are sent to the cloud (can be longer, if the value hasn't changed)

bool prevButtonState;
bool prevDoorSensor;
short visitCount = 0;                                       //keeps track of the nr of visitors         
short prevVisitCountSent = 0;                               //we only send visit count max every 30 seconds, but only if the value has changed, so keep track of the value that was last sent to the cloud.
unsigned long lastSentAt = 0;                               //the time when the last visitcount was sent to the cloud.
bool isConnected = false;                                   //keeps track of the connection state.

void setup() 
{
  pinMode(pushButton, INPUT);                               // initialize the digital pin as an input.          
  pinMode(doorSensor, INPUT);
  
  Serial.begin(SERIAL_BAUD);
  Serial1.begin(Modem.getDefaultBaudRate());                //init the baud rate of the serial connection so that it's ok for the modem
  
  prevButtonState = digitalRead(pushButton);                //set the initial state
  prevDoorSensor = digitalRead(doorSensor);                 //set the initial state
  
  Device.SetMaxSendRetry(5);                                //for this use case we don't want to get stuck too long in sending data, we primarely want to capture visits and count them, sending the count can be done later on.
}

void tryConnect()
{
  isConnected = Device.Connect(DEV_ADDR, APPSKEY, NWKSKEY);
  if(isConnected == true)
  {
     Serial.println("Ready to send data");
     //todo: improvement: retrieved the count stored on disk so that it is not lost after power down, you can use the EEPROM lib for this.
     sendVisitCount();                                  	//always send the value at initial connection, keep the platform in sync with the latest change on the device.
  } 
  else
     Serial.println("connection will by retried later");  
}


void loop() 
{
  if(isConnected == false)                                  //if we previously failed to connect to the cloud, try again now.  This technique allows us to already collect visits before actually having established the connection.
    tryConnect();
  processButton();
  processDoorSensor();
  delay(100);
  if(isConnected && prevVisitCountSent != visitCount && lastSentAt + SEND_MAX_EVERY <= millis())	// only send a message when something has changed and SEND_MAX_EVERY has been exceeded
    sendVisitCount();
}

void sendVisitCount()
{
  Serial.print("send visit count: "); Serial.println(visitCount);
  Device.Send(visitCount, INTEGER_SENSOR);                  //we also send over the visit count so that the cloud is always in sync with the device (connection could have been established after the counter was changed since last connection).
  prevVisitCountSent = visitCount;
  lastSentAt = millis();
}

// check the state of the door sensor
void processDoorSensor()
{
  bool sensorRead = digitalRead(doorSensor);                         
  if(prevDoorSensor != sensorRead)
  {
    prevDoorSensor = sensorRead;
    if(sensorRead == true)                                          //door was closed, so increment the counter 
    {
        Serial.println("door closed");
        visitCount++;                                           //the door was opened and closed again, so increment the counter
		Serial.print("VisitCount: ");Serial.println(visitCount);
    }
    else
        Serial.println("door open");
  }
}

void processButton()
{
  bool sensorRead = digitalRead(pushButton);                        // check the state of the button
  if (prevButtonState != sensorRead)                                // verify if value has changed
  {
     prevButtonState = sensorRead;
     if(sensorRead == true)                                         
     {
        Serial.println("button pressed, counter reset");
        visitCount = 0;
     }
     else
        Serial.println("button released");
  }
}

void SendValue(bool val)
{
  Serial.println(val);
  
}


void serialEvent1()
{
  Device.Process();                                                     //for future extensions -> actuators
}


