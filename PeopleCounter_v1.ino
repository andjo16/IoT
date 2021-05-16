#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

//LoRaWAN connection: __________________________________________________________________________________________

//AppEUI (lsb)
static const u1_t PROGMEM APPEUI[8] = { 0x96, 0x19, 0x12, 0x20, 0x96, 0x19, 0x12, 0x20 };
void os_getArtEui (u1_t* buf) {
  memcpy_P(buf, APPEUI, 8);
}

//EUI (lsb)
static const u1_t PROGMEM DEVEUI[8] = { 0x96, 0x19, 0x12, 0x20, 0x96, 0x19, 0x12, 0x20 };
void os_getDevEui (u1_t* buf) {
  memcpy_P(buf, DEVEUI, 8);
}

//AppKey (msb)
static const u1_t PROGMEM APPKEY[16] = { 0x20, 0x12, 0x19, 0x96, 0x20, 0x12, 0x19, 0x96, 0x20, 0x12, 0x19, 0x96, 0x20, 0x12, 0x19, 0x96 };
void os_getDevKey (u1_t* buf) {
  memcpy_P(buf, APPKEY, 16);
}

static uint8_t mydata[] = "Hello from peoplecounter!";
static osjob_t sendjob;
uint8_t counter = 0;
// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 60;

// Pin mapping
const lmic_pinmap lmic_pins = {
  .nss = 18,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 23, //maybe 14
  .dio = {/*dio0*/ 26, /*dio1*/ 33, /*dio2*/ 32}
};

//Sensors:__________________________________________________________________________________________________________
int sensor1 = 0;
int sensor2 = 0;

const int trigPin1 = 02;
const int echoPin1 = 15;
const int trigPin2 = 13;
const int echoPin2 = 12;
long duration1; // variable for the duration of sound wave travel
int distance1; // variable for the distance measurement
long duration2;
int distance2;

int nrP = 0;
int looop = 0;
int joined = 0;
const int buttonPin = 25;

struct nrP_msg_t {
  uint8_t sensor_id;
  uint16_t nrPeople;
} __attribute__((packed));
struct nrP_msg_t msg;


void printHex2(unsigned v) {
  v &= 0xff;
  if (v < 16)
    Serial.print('0');
  Serial.print(v, HEX);
}

void onEvent (ev_t ev) {
  Serial.print(os_getTime());
  Serial.print(": ");
  switch (ev) {
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
        for (size_t i = 0; i < sizeof(artKey); ++i) {
          if (i != 0)
            Serial.print("-");
          printHex2(artKey[i]);
        }
        Serial.println("");
        Serial.print("NwkSKey: ");
        for (size_t i = 0; i < sizeof(nwkKey); ++i) {
          if (i != 0)
            Serial.print("-");
          printHex2(nwkKey[i]);
        }
        Serial.println();
      }
      LMIC_setLinkCheckMode(0);
      joined = 1;
      break;

    case EV_JOIN_FAILED:
      Serial.println(F("EV_JOIN_FAILED"));
      joined = 0;
      break;
    case EV_REJOIN_FAILED:
      Serial.println(F("EV_REJOIN_FAILED"));
      joined = 0;
      break;
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      if (LMIC.txrxFlags & TXRX_ACK)
        Serial.println(F("Received ack"));
      if (LMIC.dataLen) {
        Serial.print(F("Received "));
        Serial.print(LMIC.dataLen);
        Serial.println(F(" bytes of payload"));
      }
      break;
    case EV_LOST_TSYNC:
      Serial.println(F("EV_LOST_TSYNC"));
      joined = 0;
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
      joined = 0;
      break;
    case EV_LINK_ALIVE:
      Serial.println(F("EV_LINK_ALIVE"));
      joined = 0;
      break;
    case EV_TXSTART:
      Serial.println(F("EV_TXSTART"));
      break;
    case EV_TXCANCELED:
      Serial.println(F("EV_TXCANCELED"));
      joined = 0;
      break;
    case EV_RXSTART:
      /* do not print anything -- it wrecks timing */
      break;
    case EV_JOIN_TXCOMPLETE:
      Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
      joined = 0;
      break;
    default:
      Serial.print(F("Unknown event: "));
      Serial.println((unsigned) ev);
      break;
  }
}

void do_send(osjob_t* j) {
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println(F("OP_TXRXPEND, not sending"));
  } else {
    msg.sensor_id = 0x04;
    msg.nrPeople = nrP;
    Serial.println(F("Packet queued"));
    LMIC_setTxData2(1, (uint8_t*)&msg, sizeof(msg), 0);
  }
}

void setup() {
  Serial.begin(9600);

  //LoRaWAN setup:
  os_init();
  LMIC_reset();
  do_send(&sendjob);
  joined = 0;

  //Setup sensors:
  pinMode(trigPin1, OUTPUT); // Sets the trigPin as an OUTPUT
  pinMode(echoPin1, INPUT); // Sets the echoPin as an INPUT
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);
  Serial.println("Ultrasonic Sensor HC-SR04 ready"); // print some text in Serial Monitor

  //interface
  pinMode(buttonPin, INPUT_PULLUP);

}

void loop() {
  os_runloop_once();

  if (joined == 1) {

    //Measure Sensor1
    digitalWrite(trigPin1, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin1, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin1, LOW);
    duration1 = pulseIn(echoPin1, HIGH);
    distance1 = duration1 * 0.034 / 2;

    //Measure Sensor2
    digitalWrite(trigPin2, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin2, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin2, LOW);
    duration2 = pulseIn(echoPin2, HIGH);
    distance2 = duration2 * 0.034 / 2;

    if (looop > 50) {

      //Sensor1
      if (distance1 < 10 && distance1 > 0) {
        if (sensor2 == 1) {
          if (nrP < 0 || nrP==0) { //make sure it's never below 0.
            nrP = 1;
          }
          nrP = nrP - 1;
          sensor1 = 0;
          sensor2 = 0;
          Serial.print("Person out");
          Serial.println(nrP);
          delay(1);
          looop = 0; //reset counter, to wait for people to pass.
          do_send(&sendjob);
        }
        else {
          sensor1 = 1;
          Serial.println("Sensor 1 triggered");
        }
      }

      //Sensor2
      if (distance2 < 10 && distance2 > 0) {
        if (sensor1 == 1) {
          if (nrP < 0) { //make sure it's never below 0.
            nrP = 0;
          }
          nrP = nrP + 1;
          sensor1 = 0;
          sensor2 = 0;
          Serial.print("Person in");
          Serial.println(nrP);
          delay(1);
          looop = 0; //reset counter, to wait for people to pass.
          do_send(&sendjob);
        }
        else {
          sensor2 = 1;
          Serial.println("Sensor 2 triggered");
        }
      }
    }
    else {
      looop = looop + 1;
    }
  }
}
