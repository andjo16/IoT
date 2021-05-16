/*******************************************************************************
* This file is based on an template file from the LMIC library
* It has been modified to implement the desired ventilation controller functionality
* using thermocouples and a servo-motor
*******************************************************************************/


//Lora Libraries
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

//Theromcouple Libraries
#include "Adafruit_MAX31855.h"
#include <rn2xx3.h>

//Motor Library
#include <Servo.h>

//LoRa Setup
static const u1_t PROGMEM APPEUI[8]={ 0x89, 0x67, 0x45, 0x23, 0x01, 0xEF, 0xCD, 0xAB };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

static const u1_t PROGMEM DEVEUI[8]={ 0x89, 0x67, 0x45, 0x23, 0x01, 0xEF, 0xCD, 0xAB };
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

static const u1_t PROGMEM APPKEY[16] = { 0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45, 0x67, 0X89}; //0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF };
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

static osjob_t sendjob;

const unsigned TX_INTERVAL = 1800; //30 minutes between transmissions

// Pin mapping for LoRa
const lmic_pinmap lmic_pins = {
  .nss = 18, 
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 23, //maybe 14
  .dio = {/*dio0*/ 26, /*dio1*/ 33, /*dio2*/ 32}
};

//Pin mapping for thermocouples
#define MAXDOA   35 
#define MAXCSA   14 
#define MAXCLKA  13

#define MAXDOB   22 
#define MAXCSB   23 
#define MAXCLKB  19

Adafruit_MAX31855 thermocoupleA(MAXCLKA, MAXCSA, MAXDOA);
Adafruit_MAX31855 thermocoupleB(MAXCLKB, MAXCSB, MAXDOB);

//Temperature variables
double tA;
double tB;

//Servo motor
#define SERVOPIN 21
#define ONPOSITION 110
#define OFFPOSITION 60
#define DEFAULTPOSITION 90
Servo myservo;  // create servo object to control a servo

int servoPos = 185; //start position at 0 degrees
int servoCW = 1; //clockwise rotation
int servoRun = 1;

//Button
#define BUTTON1 15 //this one is active when it is low
int button1State = 0;
int button2State = 0;
int button1Last = 0;
int button2Last = 0;

//Data packet
struct temp_msg_t{
  uint8_t sensor_idA;
  uint16_t tempA;
  uint8_t sensor_idB;
  uint16_t tempB;
  uint8_t vent_id;
  uint8_t vent_state;
  }__attribute__((packed));
struct temp_msg_t msg;

//Ventilation state variable
int vent_state;

void printHex2(unsigned v) {
    v &= 0xff;
    if (v < 16)
        Serial.print('0');
    Serial.print(v, HEX);
}

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            {
              u4_t netid = 0;
              devaddr_t devaddr = 0;
              u1_t nwkKey[16];
              u1_t artKey[16];
              LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
              Serial.print("netid: ");
              Serial.println(netid, DEC);
              Serial.print("devaddr: ");
              Serial.println(devaddr, HEX);
              Serial.print("AppSKey: ");
              for (size_t i=0; i<sizeof(artKey); ++i) {
                if (i != 0)
                  Serial.print("-");
                printHex2(artKey[i]);
              }
              Serial.println("");
              Serial.print("NwkSKey: ");
              for (size_t i=0; i<sizeof(nwkKey); ++i) {
                      if (i != 0)
                              Serial.print("-");
                      printHex2(nwkKey[i]);
              }
              Serial.println();
            }
            LMIC_setLinkCheckMode(0);
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            //Message received
            if (LMIC.dataLen) {
              Serial.print(F("Received "));
              Serial.print(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));
              for (int i = 0; i < LMIC.dataLen; i++) {
                if (LMIC.frame[LMIC.dataBeg + i] < 0x10) {
                  Serial.print(F("0"));
                }
                Serial.print(LMIC.frame[LMIC.dataBeg + i], HEX);
              }
              Serial.println();
              if(LMIC.dataLen!=2){
                Serial.println("Recieved data size not recognized (!=2)");
              }
              else{
                if(LMIC.frame[LMIC.dataBeg+0]!=msg.vent_id){
                 Serial.println("Data not targeted ventilation 0");
                }
                else{
                  if(LMIC.frame[LMIC.dataBeg+1]==0){
                    if(vent_state == 1){
                      if (tA < 150 || tB < 150) {               //Turn off the ventilation only if tA or tB less than 150°C
                        myservo.write(OFFPOSITION);              // tell servo to go to position in variable 'pos'
                        delay(1000);                       // waits 15ms for the servo to reach the position
                        myservo.write(DEFAULTPOSITION);
                        Serial.println("****State is now OFF");
                        vent_state = 0;
                        msg.vent_state = 0;
                        Serial.println(msg.vent_state);
                        do_send(&sendjob);
                      }
                    }
                  }
                  else if(LMIC.frame[LMIC.dataBeg+1]==1){
                     if(vent_state == 0){
                      if (tA > 150 || tB > 150){              //Turn on the ventilation only if tA or tB higher than 150°C
                        myservo.write(ONPOSITION);              // tell servo to go to position in variable 'pos'
                        delay(1000);                       // waits 15ms for the servo to reach the position
                        myservo.write(DEFAULTPOSITION);
                        Serial.println("****State is now ON");
                        vent_state = 1;
                        msg.vent_state = 1;
                        Serial.println(msg.vent_state);
                        do_send(&sendjob);
                      }
                    }
                  }
                  else{
                    Serial.println("Not recognized command for lghtswitch 0");
                  }
                }
              }
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
        case EV_TXSTART:
            Serial.println(F("EV_TXSTART"));
            break;
        case EV_TXCANCELED:
            Serial.println(F("EV_TXCANCELED"));
            break;
        case EV_RXSTART:
            break;
        case EV_JOIN_TXCOMPLETE:
            Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
            break;

        default:
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned) ev);
            break;
    }
}

void do_send(osjob_t* j){

    Serial.print("Internal Temp A = ");
    Serial.println(thermocoupleA.readInternal());
    Serial.print("Internal Temp B = ");
    Serial.println(thermocoupleB.readInternal());

    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        //Mesure temperature
        double tA = thermocoupleA.readCelsius();
        double tB = thermocoupleB.readCelsius();
  
        if (isnan(tA) || isnan(tB)) { //If one of the thermocouple does not work, print "Problem" in the serial monitor
            Serial.print("Problem");
        } else {
            if (vent_state == 0){ //If the ventilation system is OFF
              if (tA > 150 || tB > 150){ //And if the temperature is more than 150°C
                Serial.println("High temp");
                myservo.write(ONPOSITION);              // tell servo to switch ON the ventilation
                delay(1000);
                myservo.write(DEFAULTPOSITION);
                Serial.println("State is now ON");
                vent_state = 1;
                msg.vent_state = 1;
                Serial.println(msg.vent_state);
              }
            }
            if (vent_state == 1){ //If the ventilation system is ON
              if (tA < 150 || tB < 150){ //And if the temperature is less than 150°C
                Serial.println("Low temp");
                myservo.write(OFFPOSITION);              // tell servo to switch OFF the ventilation
                delay(1000);
                myservo.write(DEFAULTPOSITION);
                Serial.println("State is now OFF");
                vent_state = 0;
                msg.vent_state = 0;
                Serial.println(msg.vent_state);
              }
            }
            Serial.print("tA = ");
            Serial.println(tA);
            Serial.print("tB = ");
            Serial.println(tB);
            msg.sensor_idA = 0x01;
            msg.tempA = tA * 100;
            msg.sensor_idB = 0x02;
            msg.tempB = tB * 100;
            msg.vent_id = 0x03;
            
            Serial.print("sensor idA: ");
            Serial.println(msg.sensor_idA);
            Serial.print("sensor tempA: ");
            Serial.println(msg.tempA);
            Serial.print("sensor idB: ");
            Serial.println(msg.sensor_idB);
            Serial.print("sensor tempB: ");
            Serial.println(msg.tempB);
            Serial.print("vent id: ");
            Serial.println(msg.vent_id);
            Serial.print("vent state: ");
            Serial.println(msg.vent_state);
            LMIC_setTxData2(1, (uint8_t*)&msg, sizeof(msg), 0); //Send the message structure to the Cibicom network
            
        }
        Serial.println(F("Packet queued"));
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
    Serial.begin(115200);
    
    //Thermocouple initialization
    Serial.println("MAX31855 test");
    delay(500);
    Serial.print("Initializing sensor...");
    if (!thermocoupleA.begin()) {
      Serial.println("ERROR.");
      while (1) delay(10);
    }
    Serial.println("DONE.");
    
    Serial.println(F("Starting"));
    
    pinMode(BUTTON1,INPUT_PULLUP); //Initialize the button

    myservo.attach(SERVOPIN);  // attaches the servo to pin D6
    myservo.write(DEFAULTPOSITION);
    
    // LoRa Initialization
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();
    // Start job (sending automatically starts OTAA too)
    do_send(&sendjob);
    
}

void loop() {
  //LoRaWAN handling
  os_runloop_once();

  //Sense if button has been pushed
  if(!digitalRead(BUTTON1)){
    if (vent_state == 0){
      if (tA > 150 || tB > 150){            //Turn on the ventilation only if tA or tB higher than 150°C
        myservo.write(ONPOSITION);              // tell servo to switch ON the ventilation
        delay(1000);                      // waits 10ms for the servo to reach the position
        myservo.write(DEFAULTPOSITION);                 //Go back to the initial position
        Serial.println("****State is now ON");
        vent_state = 1;
        msg.vent_state = 1;
        Serial.println(msg.vent_state);
        do_send(&sendjob);              //Call the do_send function
      }
    } else if (vent_state == 1){
      if (tA < 150 || tB < 150) {               //Turn off the ventilation only if tA or tB less than 150°C
        myservo.write(OFFPOSITION);              // tell servo to switch OFF the ventilation
        delay(1000);                     // waits 10ms for the servo to reach the position
        myservo.write(DEFAULTPOSITION);                //Go back to the initial position
        Serial.println("****State is now OFF");
        vent_state = 0;
        msg.vent_state = 0;
        Serial.println(msg.vent_state);
        do_send(&sendjob);              //Call the do_send function
      }
    }
  }
}
