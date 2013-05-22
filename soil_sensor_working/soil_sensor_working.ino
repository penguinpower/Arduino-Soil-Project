#include <WiFi.h>
#include <Decagon5TE.h>
#include <SD.h>
#include <SPI.h>

Decagon5TE sensor(2, 3, 15000);
const int SD_CHIP_SELECT = 4;
const int RTC_CHIP_SELECT = 8;

const char FILENAME[] = "5TE_Data.txt";

char ssid[] = "Apartment";
char pass[] = "tenretni";
int status = WL_IDLE_STATUS;

char device_id[] = "vFB218A4E2233052";

WiFiClient client;

void setup(){
  Serial.begin(1200); // opens serial port, sets data rate to 1200 bps as per the 5TE's spec.
  
  connectToNetwork(ssid, pass); 
  
  SD_init();
  //writeToSDCard(sensor);
  //RTC_init();
  
  Serial.println("-------------------------------");
}

void loop() {
  if (sensor.isReadyForReading()){   
    sensor.readData();
    
    //SD_init();
    writeToSDCard(sensor);
   
    delay(500);
   
    boolean connected = connectToNetwork(ssid, pass); 

    if (connected){
      sendToPushingBox(sensor);
    }

    Serial.println("-------------------------------");
  }
  
  delay(1000);
}

boolean writeToSDCard(Decagon5TE sensor_data){
  //last_sd_time = millis();
 
  RTC_init();
  
  String dataString = "";
  
  dataString += ReadTimeDate();
  dataString += ",";
  dataString += doubleToString(sensor_data.getDielectricPermittivity());
  dataString += ",";
  dataString += doubleToString(sensor_data.getElectricalConductivity());
  dataString += ",";
  dataString += doubleToString(sensor_data.getTemperature());

  Serial.println("I'm going to write to the SD card");

  SPI.setDataMode(SPI_MODE0);
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("datalog.txt", FILE_WRITE);

  delay(500);

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
    Serial.println("error opening to_upload.txt");
    dataFile.close();
    return false;
  }

  //SPI.setDataMode(SPI_MODE1);  
}

boolean sendToPushingBox(Decagon5TE sensor){
  
  Serial.println("I'm going to upload to the spreadsheet");
  
  //File dataFile = SD.open("toupload.txt", FILE_READ);
  
  boolean success = false;
  
  String data = "&permittivity=";
  data += doubleToString(sensor_data.getDielectricPermittivity());
  data += "&conductivity=";
  data += doubleToString(sensor_data.getElectricalConductivity());
  data += "&temperature=";
  data += doubleToString(sensor_data.getTemperature());
  
  //char data_buffer[data.length()];  
  
  Serial.println("Starting connection to server...");
  
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
  
  //last_upload_time = millis();
  
  return success; 
}

boolean connectToNetwork(char ssid[], char password[]){
  status = WiFi.status();
  
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
  if (status) {
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());
  }
  return status;
}

String doubleToString(double input){
  char input_buffer[10] = {'/0'};
  return dtostrf(input, 2, 2, input_buffer);  
}

int RTC_init(){
  
  pinMode(RTC_CHIP_SELECT,OUTPUT); // chip select
  // start the SPI library:
  SPI.begin();
  SPI.setBitOrder(MSBFIRST); 
  SPI.setDataMode(SPI_MODE1); // both mode 1 & 3 should work 
  //set control register 
  digitalWrite(RTC_CHIP_SELECT, LOW);  
  SPI.transfer(0x8E);
  SPI.transfer(0x60); //60= disable Osciallator and Battery SQ wave @1hz, temp compensation, Alarms disabled
  digitalWrite(RTC_CHIP_SELECT, HIGH);
  delay(10);
}

boolean SD_init(){
 SPI.setDataMode(SPI_MODE0);
 Serial.println("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(SD_CHIP_SELECT)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return false;
  }
  Serial.println("card initialized.");
  return true;
}

String ReadTimeDate(){
  String temp;
  int TimeDate [7]; //second,minute,hour,null,day,month,year		
  for(int i=0; i<=6;i++){
    if(i==3)
    i++;
    digitalWrite(RTC_CHIP_SELECT, LOW);
    SPI.transfer(i+0x00); 
    unsigned int n = SPI.transfer(0x00);        
    digitalWrite(RTC_CHIP_SELECT, HIGH);
    int a=n & B00001111;    
    if(i==2){	
      int b=(n & B00110000)>>4; //24 hour mode
      if(b==B00000010)
	b=20;        
      else if(b==B00000001)
	b=10;
	TimeDate[i]=a+b;
    }
    else if(i==4){
      int b=(n & B00110000)>>4;
      TimeDate[i]=a+b*10;
    }
    else if(i==5){
      int b=(n & B00010000)>>4;
      TimeDate[i]=a+b*10;
    }
    else if(i==6){
      int b=(n & B11110000)>>4;
      TimeDate[i]=a+b*10;
    }
    else{	
      int b=(n & B01110000)>>4;
      TimeDate[i]=a+b*10;	
    }
  }
  temp.concat(TimeDate[4]);
  temp.concat("/") ;
  temp.concat(TimeDate[5]);
  temp.concat("/") ;
  temp.concat(TimeDate[6]);
  temp.concat(" ") ;
  temp.concat(TimeDate[2]);
  temp.concat(":") ;
  temp.concat(TimeDate[1]);
  temp.concat(":") ;
  temp.concat(TimeDate[0]);
  return(temp);
}
