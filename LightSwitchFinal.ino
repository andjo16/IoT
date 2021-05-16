/*******************************************************************************
 * This file is based on an template file from the LMIC library
 * It has been modified to implement the desired light switching functionality
 * using a servo-motor
 *******************************************************************************/

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <Servo.h>

//LoRa Setup
static const u1_t PROGMEM APPEUI[8]={ 0xEF,0xCD,0xAB,0x89,0x67,0x45,0x23,0x01 };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

static const u1_t PROGMEM DEVEUI[8]={ 0xEF,0xCD,0xAB,0x89,0x67,0x45,0x23,0x01 };
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

static const u1_t PROGMEM APPKEY[16] = { 0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF };
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

static osjob_t sendjob;

const unsigned TX_INTERVAL = 1800; //30 minutes between transmissions

// Pin mapping for LoRa
const lmic_pinmap lmic_pins = {
  .nss = 18, 
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 23,
  .dio = {/*dio0*/ 26, /*dio1*/ 33, /*dio2*/ 32}
};

//Data packet
struct sensor_data{
  uint8_t sensor_id;
  uint8_t state;
};
struct sensor_data lora_msg;

//Servo control
#define SERVOPIN 21
#define ONPOSITION 110
#define OFFPOSITION 60
#define DEFAULTPOSITION 90
Servo myservo;  // create servo object to control a servo

//button and LED
#define BUTTON1 2 //botton, active low
#define INTERNALLED 25 //internal led (green)
int button1State = 0;
int button1Last = 0;

//state variables
int turnedon;
int lastturnedon;

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
              //used to indicate when chip is connected to LoRaWAN
              for(int i=0; i<10; i++){
                digitalWrite(INTERNALLED, HIGH);
                delay(150);
                digitalWrite(INTERNALLED, LOW);
                delay(150);
              }

            }
            // Disable link check validation (automatically enabled
            // during join, but because slow data rates change max TX
	          // size, we don't use it in this example.
            LMIC_setLinkCheckMode(0);
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_RFU1:
        ||     Serial.println(F("EV_RFU1"));
        ||     break;
        */
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
                if(LMIC.frame[LMIC.dataBeg+0]!=lora_msg.sensor_id){
                 Serial.println("Data not targeted ligthswitch 0");
                }
                else{
                  if(LMIC.frame[LMIC.dataBeg+1]==0){
                    if(turnedon){
                      switchState();
                    }
                  }
                  else if(LMIC.frame[LMIC.dataBeg+1]==1){
                     if(!turnedon){
                      switchState();
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
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_SCAN_FOUND:
        ||    Serial.println(F("EV_SCAN_FOUND"));
        ||    break;
        */
        case EV_TXSTART:
            Serial.println(F("EV_TXSTART"));
            break;
        case EV_TXCANCELED:
            Serial.println(F("EV_TXCANCELED"));
            break;
        case EV_RXSTART:
            /* do not print anything -- it wrecks timing */
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
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        //Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, (uint8_t*)&lora_msg, sizeof(lora_msg), 0);
        Serial.println(F("Packet queued"));
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
    Serial.begin(9600);    
    Serial.println(F("Starting"));
    pinMode(BUTTON1, INPUT_PULLUP);
    pinMode(INTERNALLED, OUTPUT);

    //intially lights are on
    turnedon = 1;

    myservo.attach(SERVOPIN);  // attaches the servo to pin D6
  
    myservo.write(DEFAULTPOSITION);
    //set sensor id (recognized by backend)
    lora_msg.sensor_id = 5;

    setON(); //Reset state to ON

    //LoRa Intialization
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
  button1Last=button1State;
  button1State = !digitalRead(BUTTON1);
  lastturnedon = turnedon;
  if(button1State && !button1Last){
    //switch state when button has been pushed
    switchState();
  }
}

/**
 * Go to ON state
 * Moves motor and updates state variable
 */
void setON(){
  Serial.println("State is now ON");
  myservo.write(ONPOSITION);
  digitalWrite(INTERNALLED, HIGH);
  delay(1000);
  myservo.write(DEFAULTPOSITION);
  turnedon=1;
  lora_msg.state=1;
}

/**
 * Go to OF state
 * Moves motor and updates state variable
 */
void setOFF(){
  Serial.println("State is now OFF");
  myservo.write(OFFPOSITION);
  digitalWrite(INTERNALLED, LOW);
  delay(1000);
  myservo.write(DEFAULTPOSITION);
  turnedon=0;
  lora_msg.state=0;

}

/**
 * Switches the state from ON to OFF or the reverse
 */
void switchState(){
  if(!turnedon){
      setON();
    }
    else{
      setOFF();
    }
  //When change is made, then send message immediately to update backend
  do_send(&sendjob); 
}
