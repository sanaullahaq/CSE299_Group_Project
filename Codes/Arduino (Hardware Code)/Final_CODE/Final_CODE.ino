//Temp & Humidity Sensor
#include "DHT.h"
#define DHTPIN A0       //  pin connected to the DHT sensor
#define DHTTYPE DHT11   // sensor type DHT11
DHT dht(DHTPIN, DHTTYPE);
float humidity;
float temp;

//Ammonia
#define MQ135PIN A1    // pin
#define RL_135 20.0    //Load Resister
#define R0_135 12.09   //RZERO
#define m_135 -0.43    //slope
#define b_135 0.86     //y intercept
float sensor_value_135;
float sensor_volt_135;
float RS_gas_135;
float ratio_of_RS_R0_135;
float ppm_135;
float microgram_135;

//Carbon Monoxide
#define MQ7PIN A2      // pin
#define RL_7 10.0      //Load Resister
#define R0_7 2.31      //RZERO
#define m_7 -0.67      //slope
#define b_7 1.34       //y intercept
float sensor_value_7;
float sensor_volt_7;
float RS_gas_7;
float ratio_of_RS_R0_7;
float ppm_7;
float miligram_7;

//Optical Dust Sensor PM2.5
#define measurePIN A3            //pin
int ledPower = 12;
unsigned int samplingTime = 280;
unsigned int deltaTime = 40;
unsigned int sleepTime = 9680;
float voMeasured;
float calcVoltage;
double dustDensity;

//Thinkspeak
String apiKey = "4CUPXS6UMI3CW68H";     // ThingSpeak Channel Write API key

void setup() {
  Serial.begin(9600);                   // starting Arduino Mega with baud rate 9600
  delay(2000);                          // delay 2 seconds
  Serial1.begin(9600);                  // starting ESP-01 Wi-Fi module with baud rate 9600
  delay(2000);
  dht.begin();                          // starting DHT11 sensor

  pinMode(ledPower, OUTPUT);
  delay(2000);
  connect_WiFi();                       // function to connect our Wi-Fi module with a Wi-Fi(host)

  Serial.println("All set...Let's gooo...\n");
}

void loop() {

  //Temp & Humidity Sensor
  humidity = dht.readHumidity();    // getting humidity value from the DHT11 sensor
  Serial.print("Humidity = ");      // printing the text in the serial monitor
  Serial.print(humidity);
  Serial.println("%");
  up_Data("&field1=", humidity, "Humidity");   /* calling up_Data function to send the data to the ThingSpeak DB*/
                                               /*every field gets next data after 3.5mins(215s) in ThingSpeak*/
  Serial.println();
  delay(10000);                     // 10 seconds delay

  temp = dht.readTemperature();     // getting temp value from the DHT11 sensor
  Serial.print("Temperature = ");
  Serial.print(temp);
  Serial.println("C");
  up_Data("&field2=", temp, "Temperature"); // every field gets next data after 3.5mins(215s) in ThingSpeak
  Serial.println();
  delay(10000);

  //Ammonia
  sensor_value_135 = analogRead(MQ135PIN);                // getting analog value from MQ135 sensor
  sensor_volt_135 = sensor_value_135 * (5.0 / 1023.0);
  RS_gas_135 = ((5.0 * RL_135) / sensor_volt_135) - RL_135;
  ratio_of_RS_R0_135 = RS_gas_135 / R0_135;
  ppm_135 = (log10(ratio_of_RS_R0_135) - b_135) / m_135;  // converting the analog value into ppm 

  microgram_135 = (0.0409 * ppm_135 * 17.03) * 1000.0;  /*converting ppm unit into ug/m3*/
                                                        /*0.0409 is a constant*/
                                                        /*molecular mass of NH3 (17.03 g/mol)*/
                                                        /*(mg/m3)*1000.0*/
  if(microgram_135<0){
     microgram_135 = 0;
  }
  Serial.print("Ammonia = ");
  Serial.print(microgram_135);
  Serial.println("ug/m3");
  up_Data("&field3=", microgram_135, "Ammonia");// every field gets next data after 3.5mins(215s) in ThingSpeak
  Serial.println();
  delay(10000);


  //Carbon Monoxide
  /*almost same procedure as mQ135*/
  
  sensor_value_7 = analogRead(MQ7PIN);
  sensor_volt_7 = sensor_value_7 * (5.0 / 1023.0);
  RS_gas_7 = ((5.0 * RL_7) / sensor_volt_7) - RL_7;
  ratio_of_RS_R0_7 = RS_gas_7 / R0_7;
  ppm_7 = (log10(ratio_of_RS_R0_7) - b_7) / m_7;           //ppm

  miligram_7 =  0.0409 * ppm_7 * 28.01;                    //constant, molecular mass, mg/m3
  if(miligram_7<0){
    miligram_7 = 0;
  }
  Serial.print("Carbon Monoxide = ");
  Serial.print(miligram_7);
  Serial.println("mg/m3");
  up_Data("&field4=", miligram_7, "Carbon Monoxide");// every field gets next data after 3.5mins(215s) in ThingSpeak
  Serial.println();
  delay(10000);

  //Optical Dust Sensor PM2.5
  digitalWrite(ledPower, LOW);
  delayMicroseconds(samplingTime);
  voMeasured = analogRead(measurePIN);                // analog value
  delayMicroseconds(deltaTime);
  digitalWrite(ledPower, HIGH);
  delayMicroseconds(sleepTime);
  calcVoltage = voMeasured * (5.0 / 1023);
  dustDensity = ((0.17 * calcVoltage) - 0.1) * 1000.0;   //ug/m3

  if(dustDensity<0){
    dustDensity = 0;
  }

  Serial.print("Dust Density = ");
  Serial.print(dustDensity);
  Serial.println("ug/m3");
  up_Data("&field5=", dustDensity, "Dust Density PM2.5");   // every field gets next data after 3.5mins(215s) in ThingSpeak
  Serial.println("---------------------------------------");
  delay(10000);

/*data has been centralised(median) in thingspeak 
 *i.e for every 10 minutes all graphs show 
 *medianised value for the points.
 */
 
}

//Thinkspeak Uploding data
void up_Data(String field, float data, String field_name) {     //total delay 5+20+10=35S
  Serial1.println("AT+CIPMUX=0\r\n");                           //setting MUX = 0 for single connection            
  delay(5000);

  // TCP connection
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += "184.106.153.149";
  cmd += "\",80\r\n\r\n";

  Serial1.println(cmd);
  delay(20000);

  if (Serial1.find("ERROR")) {                      // condition to encounter "ERROR"
    Serial.println("AT+CIPSTART ERROR");
  }

  // prepare GET string (API request string)
  String getStr = "GET /update?api_key=";
  getStr += apiKey;
  getStr += field;
  getStr += data;
  getStr += "\r\n\r\n";

  // length of GET string
  cmd = "AT+CIPSEND=";
  cmd += String(getStr.length());
  cmd += "\r\n";

  Serial1.println(cmd);
  delay(10000);

  if (Serial1.find(">")) {           // condition if "True" data will uploaded successfully
    Serial1.print(getStr);
    Serial.print("Uploaded ");
    Serial.print(field_name);
    Serial.println(" succesfully");
  }
  else {                               
    Serial1.println("AT+CIPCLOSE\r\n");
    Serial.println("AT+CIPCLOSE, ERROR");
    Serial.println("Retrying...\n");
    connect_WiFi();
  }
}


void connect_WiFi() {                      //total delay 2+15= 17S
  Serial1.println("AT+CWMODE=1\r\n");      // putting our Wi-Fi module in client mode
  delay(2000);

  String cmd = "AT+CWJAP=\"";
  cmd += "Home_Sweet_Home";                 //username of host Wi-Fi
  cmd += "\",\"";
  cmd += "1731176042";                      //password
  cmd += "\"\r\n";

  Serial.println(cmd);                      // printing the text in the serial monitor
  Serial1.println(cmd);                     // sending the AT command to the Wi-Fi module
  delay(15000);

  Serial1.println("AT+CWJAP?");             // verifying whether the Wi-Fi module got connected successfully or not

  if (Serial1.find("+CWJAP")) {
    Serial.println("Connected to Wi-Fi...\n");
  }
  else {
    Serial.println("Can't connect to the Wi-Fi...");
    Serial.println("Retrying...to connect Wi-Fi...\n");
    delay(5000);
    connect_WiFi();
  }
}
