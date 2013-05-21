#include <WiFi.h>
#include <Decagon5TE.h>
#include <SD.h>

const int DRY_THRESHOLD = 30;

const unsigned long WET_SD_PERIOD = 30000;
const unsigned long DRY_SD_PERIOD = 30000;//10800000; // 3 hours in milliseconds

const unsigned long WET_UPLOAD_PERIOD = 60000; //3600000; // 1 hour in milliseconds
const unsigned long DRY_UPLOAD_PERIOD = 60000; //86400000; // 1 day in milliseconds 

unsigned long last_sd_time = 0;
unsigned long last_upload_time = 0;

unsigned long sd_period = 25000;
unsigned long upload_period = 60000;

const int chipSelect = 4;

const char FILENAME[] = "5TE_Data.txt";

char ssid[] = "Apartment";
char pass[] = "tenretni";
int status = WL_IDLE_STATUS;

char device_id[] = "vFB218A4E2233052";

WiFiClient client;

Decagon5TE sensor(2, 3, WET_SD_PERIOD);

void setup() {
  
  Serial.begin(1200); // opens serial port, sets data rate to 1200 bps as per the 5TE's spec.
  
  Serial.println("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
  
  SD.remove("datalog.txt");
  
  connectToNetwork(ssid,pass);
}

void loop() {
  
  if (sensor.isReadyForReading()){
    sensor.readData();  
  
    if (sensor.getDielectricPermittivity() >= DRY_THRESHOLD) {
      Serial.println("Wet");
      //Log to SD Card.
      sd_period = WET_SD_PERIOD;
      upload_period = WET_UPLOAD_PERIOD;
    }
    else {
      Serial.println("Dry");
      //Log when it is time to.
      sd_period = DRY_SD_PERIOD;
      upload_period = DRY_UPLOAD_PERIOD;
    }
    
    printData(sensor);
    
    boolean ready_for_sd = millis() - last_sd_time >= sd_period;
    boolean ready_for_upload = millis() - last_upload_time >= upload_period;
  
    // Check if its time to write to SD or upload to spreadsheet
    if (ready_for_sd){
      writeToSD(sensor);
      if (ready_for_upload){
          sendToPushingBox(sensor);
          //WiFi.disconnect();
      }
    }  
  }
  
  delay(1000);
}

boolean writeToSD(Decagon5TE sensor_data){
  last_sd_time = millis();
 
  Serial.println("I'm going to write to the SD card");
  
  String dataString = "";
  
  dataString += doubleToString(sensor_data.getDielectricPermittivity());
  dataString += " ";
  dataString += doubleToString(sensor_data.getElectricalConductivity());
  dataString += " ";
  dataString += doubleToString(sensor_data.getTemperature());

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("datalog.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    Serial.println(dataString);
    dataFile.close();
    return true;
    // print to the serial port too:
  }  
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
    dataFile.close();
    return false;
  }   
}

boolean connectToNetwork(char ssid[], char password[]){
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present"); 
    // don't continue:
    while(true);
  } 
  int failures = 0;
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED && failures < 2) {
  //Serial.println(failures); 
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  // Connect to WPA/WPA2 network:    
  status = WiFi.begin(ssid, pass);
  ++failures;
  }
  
  return status;
}

//---------------------
// PushingBox Stuff
//---------------------

boolean sendToPushingBox(Decagon5TE sensor_data){
  
  Serial.println("I'm going to upload to the spreadsheet");
  
  boolean success = false;
  
  String data = "&permittivity=";
  data += doubleToString(sensor_data.getDielectricPermittivity());
  data += "&conductivity=";
  data += doubleToString(sensor_data.getElectricalConductivity());
  data += "&temperature=";
  data += doubleToString(sensor_data.getTemperature());
  
  //char data_buffer[data.length()];  
  
  Serial.println("\nStarting connection to server...");
  
  if (client.connect("api.pushingbox.com", 80)) {
    Serial.println("connected to server");
    // Make a HTTP request:
    //client.println("GET /pushingbox?devid=vFB218A4E2233052&permittivity=1.52&conductivity=23.02 HTTP/1.1");
    client.print("GET /pushingbox?devid=");
    client.print(device_id);
    client.print(data);
    client.println(" HTTP/1.1");
    client.println("Host: api.pushingbox.com");
    client.println("Connection: close");
    client.println();
    
    success = true;
    Serial.println("Upload Successful");
  }
  else {
    success = false;  
    Serial.println("Upload Failed");
  }
  
  client.flush();
  client.stop();
  
  last_upload_time = millis();
  
  return success; 
  return true;
}

String doubleToString(double input){
  char input_buffer[10] = {'/0'};
  return dtostrf(input, 2, 2, input_buffer);  
}

void printData(Decagon5TE sensor_data){
  Serial.print("\tDielectric Permittivity: ");
  Serial.println(sensor_data.getDielectricPermittivity());
    
  Serial.print("\tElectrical Conductivity: ");
  Serial.print(sensor_data.getElectricalConductivity());
  Serial.println(" dS/m");
    
  Serial.print("\tTemperature: ");
  Serial.print(sensor_data.getTemperature());
  Serial.println(" C");  
}
