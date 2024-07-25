#include <WaziDev.h>
#include <xlpp.h>
#include <DHT.h>
/**************** Add Your Code Here ********************/
#define ECHO_PIN 3  
#define TRIG_PIN 4

#define DHT_PIN 2 // DHT digital pin
/********************************************************/

/*
   NwkSKey (Network Session Key) and Appkey (AppKey) are used for securing LoRaWAN transmissions.
   You need to copy them from/to your LoRaWAN server or gateway.
   You need to configure also the devAddr. DevAddr need to be different for each devices!!
*/

// Copy'n'paste the key to your Wazigate: 774 AF 7F 0B 8D C4 12 CC 3D 55 60 50 0B 64 6B 95
// 85 96 9B 4F D5 CD 93 12 E3 F9 53 D3 38 EF 50 77
unsigned char nwkSkey[16] = { 0x85, 0x96, 0x9B, 0x4F, 0xD5, 0xCD, 0x93, 0x12, 0xE3, 0xF9, 0x53, 0xD3, 0x38, 0xEF, 0x50, 0x77 };
// Copy'n'paste the key to your Wazigate: 74 AF 7F 0B 8D C4 12 CC 3D 55 60 50 0B 64 6B 95
unsigned char appSkey[16] = { 0x85, 0x96, 0x9B, 0x4F, 0xD5, 0xCD, 0x93, 0x12, 0xE3, 0xF9, 0x53, 0xD3, 0x38, 0xEF, 0x50, 0x77 };
// Copy'n'paste the DevAddr (Device Address): 0B 64 6B 95
unsigned char devAddr[4] = {0x38, 0xEF, 0x50, 0x77};
// You can change the Key and DevAddr as you want.


/**************** Add Your Code Here ********************/
WaziDev wazidev;

DHT dht(DHT_PIN, DHT11);

float distance_centimetre() {
  // Send sound pulse
  digitalWrite(TRIG_PIN, HIGH); // pulse started
  delayMicroseconds(12);
  digitalWrite(TRIG_PIN, LOW); // pulse stopped

  // listen for echo 
  float tUs = pulseIn(ECHO_PIN, HIGH); // microseconds
  float distance = tUs / 58; // cm 
  return distance;
}
/*******************************************************/

void setup()
{
  Serial.begin(115200);
  wazidev.setupLoRaWAN(devAddr, appSkey, nwkSkey);

  /**************** Add Your Code Here ********************/
  pinMode(TRIG_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);
  pinMode(ECHO_PIN, INPUT);
 
  dht.begin(); // start DHT sensor
  delay(1000);
  /**************** Add Your Code Here ********************/
}

XLPP xlpp(120);

uint8_t uplink()
{
  uint8_t e;

  /**************** Add Your Code Here ********************/
  // 1.
  // Read sensor values.
  float humidity = dht.readHumidity(); // %
  float temperature = dht.readTemperature(); // °C
  float distance = distance_centimetre();
  /********************************************************/
  // 2.
  // Create xlpp payload for uplink.
  xlpp.reset();
  
  /**************** Add Your Code Here ********************/
  // Add sensor payload
  xlpp.addRelativeHumidity(0, humidity);
  xlpp.addTemperature(1, temperature);
  xlpp.addDistance(2, distance);

  Serial.print("Temp: ");
  Serial.print(temperature);
  Serial.print("C, Humid: ");
  Serial.print(humidity);
  Serial.print("%, Dist: ");
  Serial.print(distance);
  Serial.println("cm.");
  /********************************************************/
  
  // 3.
  // Send payload uplink with LoRaWAN.
  serialPrintf("LoRaWAN send ... ");
  e = wazidev.sendLoRaWAN(xlpp.buf, xlpp.len);
  if (e != 0)
  {
    serialPrintf("Err %d\n", e);
    return e;
  }
  serialPrintf("OK\n");
  return 0;
}

uint8_t downlink(uint16_t timeout)
{
  uint8_t e;

  // 1.
  // Receive LoRaWAN downlink message.
  serialPrintf("LoRa receive ... ");
  uint8_t offs = 0;
  long startSend = millis();
  e = wazidev.receiveLoRaWAN(xlpp.buf, &xlpp.offset, &xlpp.len, timeout);
  long endSend = millis();
  if (e)
  {
    if (e == ERR_LORA_TIMEOUT)
      serialPrintf("nothing received\n");
    else 
      serialPrintf("Err %d\n", e);
    return e;
  }
  serialPrintf("OK\n");
  
  serialPrintf("Time On Air: %d ms\n", endSend-startSend);
  serialPrintf("LoRa SNR: %d\n", wazidev.loRaSNR);
  serialPrintf("LoRa RSSI: %d\n", wazidev.loRaRSSI);
  serialPrintf("LoRaWAN frame size: %d\n", xlpp.offset+xlpp.len);
  serialPrintf("LoRaWAN payload len: %d\n", xlpp.len);
  serialPrintf("Payload: ");
  if (xlpp.len == 0)
  {
    serialPrintf("(no payload received)\n");
    return 1;
  }
  printBase64(xlpp.getBuffer(), xlpp.len);
  serialPrintf("\n");

  // 2.
  // Read xlpp payload from downlink message.
  // You must use the following pattern to properly parse xlpp payload!
  int end = xlpp.len + xlpp.offset;
  while (xlpp.offset < end)
  {
    // [1] Always read the channel first ...
    uint8_t chan = xlpp.getChannel();
    serialPrintf("Chan %2d: ", chan);

    // [2] ... then the type ...
    uint8_t type = xlpp.getType();

  }
}

void loop(void)
{
  // error indicator
  uint8_t e;

  // 1. LoRaWAN Uplink
  e = uplink();
  // if no error...
  if (!e) {
    // 2. LoRaWAN Downlink
    // waiting for 6 seconds only!
    downlink(6000);
  }

  serialPrintf("Waiting 1min ...\n");
  delay(60000);
}
