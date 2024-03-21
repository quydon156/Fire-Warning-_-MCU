// ESP32 open server on port 10000 to receive a float

#include <WiFi.h>
#include <HardwareSerial.h>
#include "DHT.h"

// DHT11
#define DHTPIN 4     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 11

int speed = 0;
//UART
HardwareSerial SerialPort(0); // use UART2

// Replace with your network credentials
const char* ssid = "T3";
const char* password = "Quydon156";

char Fire_Detected = 'N';

WiFiServer server(10000);  // server port to listen on

//DHT func
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  //UART Pin
  SerialPort.begin(115200, SERIAL_8N1, 3, 1); 
  //DHT Open communicate
  dht.begin();

  // setup Wi-Fi network with SSID and password
  SerialPort.printf("Connecting to %s\n", ssid);
  SerialPort.printf("\nattempting to connect to WiFi network SSID '%s' password '%s' \n", ssid, password);
  // attempt to connect to Wifi network:
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    SerialPort.print('.');
    delay(500);
  }
  server.begin();
  // you're connected now, so print out the status:
  printWifiStatus();
  //Serial.println(" listening on port 10000");
  //SerialPort0th.println("Co len don oi, cau lam duoc ma");
}

boolean alreadyConnected = false;  // whether or not the client was connected previously

void loop() {
  Fire_Detected = 'N';
  static WiFiClient client;
  static int16_t seqExpected = 0;
  if (!client)
    client = server.available();  // Listen for incoming clients
  if (client) {                   // if client connected
    if (!alreadyConnected) {
      // clead out the input buffer:
      client.flush();
      SerialPort.println("We have a new client");
      alreadyConnected = true;
    }
    // if data available from client read and display it
    int length;
    float value;
    if ((length = client.available()) > 0) {
      //str = client.readStringUntil('\n');  // read entire response
      //Serial.printf("Received length %d - ", length);
      // if data is correct length read and display it
      if (length == sizeof(value)) {
        client.readBytes((char*)&value, sizeof(value));
        //SerialPort.printf("value %f \n", value);
      } else
        while (client.available()) SerialPort.print((char)client.read());  // discard corrupt packet
      
    // send msg
    if (value == 1 )
      {
      //SerialPort.print("1");
      Fire_Detected = 'F';
      }
    if (value == 0)
      {
      //SerialPort.print("0");
      Fire_Detected = 'N';
      }
    //SerialPort.printf("%f",value);
    //SerialPort.printf("%c\n",Fire_Detected);
    } 
    // RESPONSES about ~350ms per signal
  }

  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    SerialPort.println("Not Connected DHT11");
    SerialPort.printf("%c",Fire_Detected);
    return;
  }
  if (!isnan(h) || !isnan(t)) {
    if(Fire_Detected == 'F'){
        SerialPort.printf("%.1f%.1fFE", h, t);
    }
    if(Fire_Detected == 'N'){
        SerialPort.printf("%.1f%.1fNE", h, t);
    }
  
  }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("\nSSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
