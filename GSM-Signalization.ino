#include <Wtv020sd16p.h>      // Library for audio module
#include <SoftwareSerial.h>   // Library for program realization UART 

#define zoneBuild  A1         // For zone of fence in the yard

#define onSignalIndicate  7   // For on/off signalization indication

// For audio module
int resetPin = 2;             // The pin number of the reset pin.
int clockPin = 3;              // The pin number of the clock pin.
int dataPin = 4;              // The pin number of the data pin.
int busyPin = 5;              // The pin number of the busy pin.

String _response = "";  // Variable for saving module's answer

Wtv020sd16p wtv020sd16p(resetPin,clockPin,dataPin,busyPin);

SoftwareSerial SIM800(8, 9);  // RX, TX

void setup() {
  
  pinMode(onSignalIndicate, OUTPUT);            // Set On signalizatioon after first turn on the power
  digitalWrite(onSignalIndicate, HIGH);
  
  pinMode(zoneBuild, INPUT_PULLUP);             // Set pull to 5V zone pin

  wtv020sd16p.reset();
  Serial.begin(9600);                           // Speed of data changing with PC
  SIM800.begin(9600);                           // Speed of data changing with GSM-module 
  Serial.println("Start!");

  sendATCommand("AT", true);                    // Set up speed
  sendATCommand("ATS0=1", true);
  _response = sendATCommand("AT+DDET=1,0,0", true);   // Turn on DTMF
  _response = sendATCommand("AT+COLP=1", true);  // Set up answer on outcoming ring, only after user answer 
  _response = sendATCommand("AT+CMGF=1;&W", true); // Turn on SMS (Text mode) and immediately saving (AT&W)!

}

String sendATCommand(String cmd, bool waiting) {
  String _resp = "";                            // For saving result
  Serial.println(cmd);                          // Dublicate comman in port
  SIM800.println(cmd);                          // Send command to GSM-module
  if (waiting) {                                // Waiting for answer
    _resp = waitResponse();                     // ... wating when answer will be pass
    
    if (_resp.startsWith(cmd)) {                // Delete dublicate command from answer
      _resp = _resp.substring(_resp.indexOf("\r", cmd.length()) + 2);
    }
    Serial.println(_resp);                      // Dublicate answer to port
  }
  return _resp;                                 // Return result
}

String waitResponse() {                         // Function for waiting response and return result
  String _resp = "";                            // For saving result
  long _timeout = millis() + 10000;             // For timeout 10 s
  while (!SIM800.available() && millis() < _timeout)  {}; // Waite for response 10 s ...
  if (SIM800.available()) {                     // If avaliable
    _resp = SIM800.readString();                // ... Read and save
  }
  else {                                        // If timeout ...
    Serial.println("Timeout...");             
  }
  return _resp;                                
}

// Отдельная функция для логики DTMF
String result = "";                             // For output data
void processingDTMF(String symbol) {
  Serial.println("Key: " + symbol);             // Output Serial for cotrol
  if (symbol == "1") {
        Serial.println("Signall On");
        digitalWrite(onSignalIndicate, HIGH);
        //set = true;
        wtv020sd16p.playVoice(1);
        //call = true;
        delay(10000);
        SIM800.println("ATH");
      }
  if (symbol == "0") {
        Serial.println("Signall Off");
        digitalWrite(onSignalIndicate, LOW);
        wtv020sd16p.playVoice(2);
      }
}

void loop() {
  if ((digitalRead(zoneBuild) == HIGH) && (digitalRead(onSignalIndicate) == HIGH)) {
    volatile int callTime = millis() + 5000;
    sendATCommand("ATD+380936467813;", true);
    WaitResponse:
    int waiting = 0;
    while(waiting < 3) {
      _response = waitResponse(); 
      _response.trim();
      if (_response.startsWith("+COLP:")) {
        Serial.println("Play voice3");
        wtv020sd16p.playVoice(3);
        delay(7000);
        wtv020sd16p.playVoice(3);
        delay(7000);
        wtv020sd16p.playVoice(3);
        delay(7000);
        digitalWrite(onSignalIndicate, LOW);
        break;
      } 
      waiting++;
    }
      sendATCommand("ATH", true);
      Serial.println("Fall");
  }
  
  if (SIM800.available())   {                   // Waite response
    _response = waitResponse();                 // Get response
    Serial.println(">" + _response);            
    int index = -1;
    do  {                                       // iterate line by line every answer that came
      index = _response.indexOf("\r\n");        // Get the line feed index
      String submsg = "";
      if (index > -1) {                         // If there is a line break, then
        submsg = _response.substring(0, index); // Getting first line
        _response = _response.substring(index + 2); // And remove it from the pack
      }
      else {                                    // If there are no more transfers
        submsg = _response;                     // The last line is all that's left of the pack.
        _response = "";                         // The pack is reset
      }
      submsg.trim();                            // Remove white space characters to the right and left
      if (submsg != "") {                       // If the string is significant (not empty), then we already recognize it
        Serial.println("submessage: " + submsg);
        if (submsg.startsWith("+DTMF:")) {      // If the answer starts with "+ DTMF:" then:
          String symbol = submsg.substring(7, 8);  // Pull out a character from 7 positions with a length of 1 (8 each)
          processingDTMF(symbol);               // For convenience we bring logic to a separate function.
        }
        else if (submsg.startsWith("BUSY") || submsg.startsWith("NO CARRIER")) {
          Serial.println("Done? signal Off");
          digitalWrite(onSignalIndicate, LOW);
        }
        else if (submsg.startsWith("RING")) {   // When an incoming call ...
          sendATCommand("ATA", true);           // ...answer (pick up)
          wtv020sd16p.playVoice(0);
        }
      }
    } while (index > -1);                       // As long as the line break index is valid
  }
  if (Serial.available())  {                    // Expect on Serial command...
    SIM800.write(Serial.read());                // ...and send the received command to the modem
  };
}

void sendSMS(String phone, String message)
{
  sendATCommand("AT+CMGS=\"" + phone + "\"", true);             // Go to the text message input mode
  sendATCommand(message + "\r\n" + (String)((char)26), true);   // After the text we send a line break and Ctrl + Z
}
