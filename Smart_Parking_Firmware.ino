
#define modem_power_pin 13 //Modem Power Button pin
#define BAT_DIV A1  

#define trigPin1 13
#define echoPin1 12
#define red1 11
#define green1 10 

#define trigPin2 9
#define echoPin2 8
#define red2 7
#define green2 6
volatile bool modem_on = false;

void setup() {
  Serial1.begin(9600);
  Serial.begin(9600);
  pinMode(trigPin1, OUTPUT);
  pinMode(modem_power_pin, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(red1, OUTPUT); 
  pinMode(green1, OUTPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);
  pinMode(red2, OUTPUT); 
  pinMode(green2, OUTPUT);
  Serial.println("PROG START");
}

void loop() {
  unsigned long timers = millis();
//  while ((millis() - timers) < 30000) {}
//  digitalWrite(LED, HIGH);
  upload_data();
}

long get_sensor_1(int trigPin,int echoPin) {
  // TODO: Write your sensor stuff here
 long duration;
  long distance;
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration/2) / 29.1;
  return distance;

}

long get_sensor_2(int trigPin,int echoPin) {
    // TODO: Write your sensor stuff here
   long duration;
  long distance;
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration/2) / 29.1;
  return distance;

}

boolean upload_data() {
  Serial.println(F("UPLOAD"));
  if(modem_power_on()) //turn on the modem
  {    
    if (!check_network()) 
    { //check that the network has registerd
      Serial.println(F("NETWORK ERROR"));
      modem_power_off(); //shut down the mode
      // last_data_send_time = nowRTC(); //set the upload time flag.
      return false;
    } 
    else 
    {
      delay(500); //do we need to check for a real data connection??
    
      if(initate_connection()) //connect the SIM800 to the 3G network. 
      {
        if(!send_data()) //send the upande data
           // writeErrorLog("Upload Fail"); // TODO: Debug this point
        
        query_modem(F("HTTPTERM")); //term http service (always do this to prevent a lockup)
  
        modem_power_off(); //shut down the modem
        // last_data_send_time = nowRTC(); //set the upload time flag.

        return true;
      }
    }
  }

  // last_data_send_time = nowRTC(); //set the upload time flag.
  modem_power_off(); //shut down the mode
  return false;
}

int getVoltage() {
  int BAT = analogRead(BAT_DIV   );
  return BAT;
}

boolean modem_power_on()
{ 
  Serial.println(F("Starting Modem"));
  if(getVoltage() < 300) //assuming that the higher of the cells is connected to the modem when near the limit of 3.4v
  {
    Serial.println(F("modemV too low")); // TODO: debug this point
    // writeErrorLog("ModemVLow"); 
    // modem_lvdo=true;
    return false;
  } 
 
  
  Serial1.begin(19200);
  for(int i = 0; i<3; i++)
  {
    // if(digitalRead(modem_raw_power) == HIGH)
    // {
    //   digitalWrite(modem_raw_power, LOW);
    //   delay(1000);
    // }   TODO: Do we need this commented out part?
    
    if(!modem_on)
    {
      pinMode(modem_power_pin, OUTPUT);
      delay(500);
      digitalWrite(modem_power_pin, HIGH);
      delay(100);
      digitalWrite(modem_power_pin,LOW); //pull the pin low for at least 1000ms. 
      delay(1200);
      digitalWrite(modem_power_pin,HIGH);
      delay(3000);
    }
 
    Serial1.println(F("AT"));
      
    if(modem_response())
    {
//      debug_message(F("Modem On")); //TODO: Debug this point
      modem_on = true;
        
      return true;  
    }
   delay(1000);
   modem_on=false; 
  }
//  writeErrorLog("SD Power Error"); // TODO: Add when logging on SD
  return false;
}


void modem_power_off()
{
  if(modem_on)
  { 
    pinMode(modem_power_pin, OUTPUT);  
     digitalWrite(modem_power_pin, HIGH);
      delay(100);
      digitalWrite(modem_power_pin,LOW); //pull the pin low for at least 1000ms. 
      delay(1200);
      digitalWrite(modem_power_pin,HIGH);
      delay(500);
      pinMode(modem_power_pin, INPUT); //put the pin to high Z
  }
  modem_on=false; // TODO: create 
  Serial1.end(); //disable the serial port to prevent current draw.
}

bool check_network() // Must only be used after toggle_modem_power
{
  long __timeout = millis();
  while((millis()-__timeout)<10000)
  {
     Serial1.println(F("AT+CREG?"));
     String __ATResponse = Serial1.readString();
     char __StatValue='0';
     if(__ATResponse.indexOf("+CREG"))
     {
        __StatValue = __ATResponse.charAt(__ATResponse.indexOf(",")+1);
     }
  
      if ( (__StatValue == '1') | (__StatValue == '5') )
        return true;
      delay(200); 
  }
  // writeErrorLog("No Sig");
  return false;     
}

boolean initate_connection() {
   
  Serial.println(F("Initiate modem"));
  if(!query_modem(F("SAPBR=3,1,\"CONTYPE\",\"GPRS\""))) return false;

  Serial1.print(F("AT+SAPBR=3,1,\"APN\",\""));
  Serial1.print(F("Safaricom")); // TODO: replace this with network provider APN
  Serial1.println(F("\""));
  if (!modem_response()) 
  {
    Serial.println(F("APN fail")); // TODO: DEBUG this
    return false; 
  }
//  TODO: This should be commented out if using safaricom
    
    Serial1.print(F("AT+SAPBR=3,1,\"USER\",\""));
    Serial1.print("saf");
    Serial1.println(F("\""));
    if (!modem_response()) 
    {
      Serial.println(F("SAPBR USER fail"));
      return false; 
    }
    
    Serial1.print(F("AT+SAPBR=3,1,\"PWD\",\"")); 
    Serial1.print("data");
    Serial1.println(F("\""));
    if (!modem_response()) 
    {
      Serial.println(F("SAPBR PWD fail"));
      return false; 
    }
 

  query_modem(F("SAPBR=1,1")); //does this need an if return?
  if(!query_modem(F("SAPBR=2,1"))) return false;
  
  return true;
}

boolean send_data()
{
    Serial.println(F("SEND")); // TODO: Debug this point
    int __errorCount = 0;
    // long __filePosition = 0; // TODO: for logging
    //TOOD: send up comment if no data - or dead SD card
    
    //get signal strength
    
    Serial1.println(F("AT+CSQ"));
    unsigned long __actionTimer = millis(); //start the countdown
    int __signalStrength = -1;
    while(((__actionTimer - millis()) < 10000) & (__signalStrength == -1))
    {
      __signalStrength = Serial1.parseInt();
    }
   
    // while(__filePosition != -1)  //while we still have lines to send // TODO: restore while loop if logging
    // {
//      __filePosition = SDReadNextLineStartPosn(temp_file_name); //get the index of the next viable line;
//      String __SDLine = SDReadLine(temp_file_name,__filePosition); //read the line in from the SD card.
// TODO: after adding SD logging
       //intiate http mode
      if(!query_modem(F("HTTPINIT"))) return false; //start an HTTP session
      if(!query_modem(F("HTTPPARA=\"CID\",1"))) return false; //set the SIM800 into HTTP POST mode

      Serial1.print(F("AT+HTTPPARA=\"URL\",\""));  //set the post to server
      Serial1.print(F("https://api.thingspeak.com/channels.json")); // TODO: Replace this with Thingspeak URL key
      Serial1.println(F("\""));
      modem_response();
      
      Serial1.println(F("AT+HTTPPARA=\"CONTENT\",\"application/x-www-form-urlencoded\""));  //set the post concent type to server
      Serial1.flush();
      modem_response(); 
      delay(10);
      //Generate the string to send (we need to make a big string because we need to find the length before shipping out the data. blerg!) 
      
      //field1=pressure
      //field2=temperature
      //field3=humidity
      //field4=bat_voltage
      //field5=signal strength
      
    
      String __uploadData = "api_key=";
      __uploadData += "SIT1QUNU6O9TX8VW"; // TODO: replace this with Thingspeak API_KEY
      __uploadData += "&field1=";
      __uploadData += String(get_sensor_1(trigPin1, echoPin1));
      __uploadData += "&field2=";
      __uploadData += String(get_sensor_2(trigPin1, echoPin2));


      int __dataLength = __uploadData.length(); //get the length of the data that we are sending
      Serial.println(__uploadData);
      Serial.println(__dataLength);
      Serial1.print(F("AT+HTTPDATA="));  Serial1.print(__dataLength); Serial1.println(F(",20000")); //set up the data size and the max time
     
      boolean __modemRet = Serial1.find((char *)"DOWNLOAD");
      if (!__modemRet) {
        Serial.println(F("DOWNLOAD OK fail")); // TODO: Debug this point
        return false; 
      }
    
      Serial1.print(__uploadData);  //upload the data! 

      __modemRet = Serial1.find((char *)"OK");
      if (!__modemRet) {
        Serial.println(F("DOWNLOAD OK fail")); // TODO: Debug this point
        return false; 
      }
     
      if(!query_modem(F("HTTPACTION=1"))) return false; //Set the HTTP Action (1 is POST) and do it! 
     
      // Wait for post to complete (Should retunr HTTPACTION 1,200,0)
      __actionTimer = millis(); //start the countdown
      while (millis() - __actionTimer < 60000) //give it 60 seconds to do so! 
      {
        if (Serial1.find((char *)"+HTTPACTION:")) {
          break;
        } else 
        {
          delay(100); //try again in 100ms
        }  
      }

      if (millis() - __actionTimer >= 60000)
      {
       // Will return ERROR anyway because we HTTPACTION has not responded yet
       Serial1.println("cant find +AC");
       if(!query_modem(F("HTTPTERM"))) return false;
      }
       
      int __retCode = Serial1.parseInt();
      __retCode = Serial1.parseInt(); //get the second number! 
      Serial1.println(__retCode);
      if (__retCode == 200) 
      {  
       
         Serial1.println(F("AT+HTTPREAD")); //start the HTTP REad       
         // SDDeleteLine(temp_file_name,__filePosition);     
      
      }
      else
      {
        __errorCount++; //if we get more than 10 errors then die until the next upload period. lets not get in to an infinite loop!
        // writeErrorLog("SEND ERROR " +  String(__retCode));
        if(__errorCount < 5)
        {
          return false;
        }
      }
      
      // __filePosition = SDReadNextLineStartPosn(temp_file_name); //get the index of the next viable line // TODO: return this when we include logging
   // } //end of while
      
      
   // //if there are no more viable lines then delete the file! 
   //  if(__filePosition == -1) 
   //  {
   //    //no more data so remove file
   //    SD.begin();
   //    Fat16::init(&SD);
   //    file.remove(temp_file_name.c_str());
   //  } // TODO: This is also part of logging
       
    Serial1.println(F("AT+HTTPTERM")); //terminate the HTTP session
    return true;
}

boolean query_modem(const __FlashStringHelper* command, char* __true_result)
{
  Serial1.print(F("AT+"));
  Serial1.println(command);
  Serial1.flush();
  Serial.println(command); // TODO: debug this
  if(!modem_response(__true_result))
  {
   // writeErrorLog(command + ":error");
    Serial.println("false"); // TODO: Debug this
    return false;
  }
  Serial.println("true"); // TODO: Debug this
  return true;
}

boolean query_modem(const __FlashStringHelper* command)
{
  Serial1.print(F("AT+"));
  Serial1.println(command);
  Serial1.flush();
//  debug_message(command,1); //output the command to the debug console // TODO: Debug this point
  if(!modem_response())
  {  
   // writeErrorLog(command + ":error"); 
    Serial.println("false"); // TODO: Debug this
    return false;
  }
//  debug_message("true",1); // TODO: Debug this // TODO: Debug this point
  return true;
}

boolean modem_response()
{ 
  return modem_response("OK");
}

boolean modem_response(char* __true_result)
{
  long __timeout = millis();
  while((__timeout - millis())<3000) //all 3 seconds to get the resoponse
  {
    if(Serial1.find(__true_result)) //find the OK signal... it actually dies when it gets to the end of the buffer
    {
      delay(100);  
      while(Serial1.available() > 0) //runout the incoming buffer
      {
        Serial1.read();
      }
       return true;
    }
   
   }
   return false;
}
