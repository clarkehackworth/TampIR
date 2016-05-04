// TampIR v.02
// Jeff Beard-Shouse
// Date:

#include <IRremote.h>
#include <SerialCommand.h> //Note needs to be modified to increase buffer size



#define arduinoLED 13   // Arduino LED on board

SerialCommand sCmd;     

IRsend irsend;

int RECV_PIN = 11;

IRrecv irrecv(RECV_PIN);

decode_results results;

int mode = 0;  //0 - recieve dump
               //1 - modify and send

boolean dumpShowHex=true;
boolean dumpShowRaw=false;
boolean dumpShowBin=false;

unsigned long tmp = 0;

void setup() {
  pinMode(arduinoLED, OUTPUT);      // Configure the onboard LED for output
  digitalWrite(arduinoLED, LOW);    // default to LED off

  Serial.begin(9600);

  // Setup callbacks for SerialCommand commands
  sCmd.addCommand("dump", modeDump);
  sCmd.addCommand("trans", trans);
  sCmd.addCommand("fuzz", modeFuzz);
  sCmd.setDefaultHandler(unrecognized);      // Handler for command that isn't matched  (says "What?")
  Serial.println("TampIR Ready");
}

void loop() {
  sCmd.readSerial();     // We don't do much, just process serial commands
  
  if(mode==0){
   if (irrecv.decode(&results)) {
    //Serial.println(results.value, HEX);
    dump(&results);
    irrecv.resume(); // Receive the next value
   }
  }
  
  /*Serial.print(tmp,BIN);
  Serial.print(" - ");
  unsigned long tmp2 = tmp;
  tmp2 = tmp >> 1;
  Serial.println(tmp2,BIN);
  tmp=tmp+1;*/
}



// Dumps out the decode_results structure.
// Call this after IRrecv::decode()
// void * to work around compiler issue
//void dump(void *v) {
//  decode_results *results = (decode_results *)v
void dump(decode_results *results) {
  int count = results->rawlen;
  if (results->decode_type == UNKNOWN) {
    Serial.print("Unknown encoding: ");
  } 
  else if (results->decode_type == NEC) {
    Serial.print("Decoded NEC: ");
  } 
  else if (results->decode_type == SONY) {
    Serial.print("Decoded SONY: ");
  } 
  else if (results->decode_type == RC5) {
    Serial.print("Decoded RC5: ");
  } 
  else if (results->decode_type == RC6) {
    Serial.print("Decoded RC6: ");
  }
  else if (results->decode_type == PANASONIC) {	
    Serial.print("Decoded PANASONIC - Address: ");
    Serial.print(results->panasonicAddress,HEX);
    Serial.print(" Value: ");
  }
  else if (results->decode_type == JVC) {
     Serial.print("Decoded JVC: ");
  }
  if(dumpShowHex){
    Serial.print(results->value, HEX);
    Serial.print(" (");
    if(results->bits<10)
       Serial.print(" "); //addjust if only single digit bits (formatting will not work properly for >=100 bits)
    Serial.print(results->bits, DEC);
    Serial.print(" bits) ");
  }
  if(dumpShowBin){
    Serial.print(results->value, BIN);
    Serial.print(" (");
    if(results->bits<10)
       Serial.print(" "); //addjust if only single digit bits (formatting will not work properly for >=100 bits)
    Serial.print(results->bits, DEC);
    Serial.print(" bits) ");
  }
  if(dumpShowRaw){
    Serial.println("");
    Serial.print("Raw (");
    Serial.print(count, DEC);
    Serial.print("): ");

    for (int i = 0; i < count; i++) {
      if ((i % 2) == 1) {
        Serial.print(results->rawbuf[i]*USECPERTICK, DEC);
      } 
      else {
        Serial.print(-(int)results->rawbuf[i]*USECPERTICK, DEC);
      }
      Serial.print(" ");
    }

  }
  Serial.println("");
}

void trans(){
  
  if(mode!=1)
    Serial.println("Setting mode trans");
  mode=1;

  char *arg = sCmd.next();
  
  char *valueArg = sCmd.next();
  char *lengthArg = sCmd.next();

  transmitString(arg,valueArg,lengthArg);
    
}

void transmitString(char *manArgChar, char *valueArg,char *lengthArg){
  String manArg = String(manArgChar);
  
 unsigned long code=0;
  if(valueArg[1]=='x') //if hex format
    code = strtoul(valueArg, NULL, 16);
  else
    code = strtoul(valueArg, NULL, 2);
  
  if(!manArg.equals("") && valueArg!=NULL && lengthArg!=NULL){
    if(manArg.equals("NEC")){
      Serial.println("Sending NEC: "+String(code,16)+", "+String(atoi(lengthArg)));
      irsend.sendNEC(code,atoi(lengthArg));
    }
    if(manArg.equals("RC5")){
      Serial.println("Sending RC5: "+String(code,16)+", "+String(atoi(lengthArg)));
      irsend.sendRC5(code,atoi(lengthArg));
    }
    //TODO: add other manufacturer calls
    
  } 
}


void modeDump(){
  Serial.println("Setting mode dump");
  mode=0; 
  irrecv.enableIRIn(); // Start the receiver

  char *arg = sCmd.next();
  if(arg!=NULL){
   dumpShowBin=false;
   dumpShowRaw=false;
   dumpShowHex=false; 
  }
  while( arg != NULL){
    String strArg = String(arg);
    if (strArg.equals("bin")) 
       dumpShowBin=true;  
    if (strArg.equals("hex")) 
       dumpShowHex=true;  
    if (strArg.equals("raw")) 
       dumpShowRaw=true;  
    arg = sCmd.next();
  }
  
}


void modeFuzz(){
  if(mode!=2)
      Serial.println("Setting mode fuzz");
  mode=2;

  int places[32];
  int placesLength =0;

  char *arg = sCmd.next();
  String manArg = String(arg);
  char *valueArg = sCmd.next();
  char *lengthArg = sCmd.next(); 
  char *delayArg = sCmd.next(); 
  int delaySec = atoi(delayArg); 
  
  for(int i=0;i<strlen(valueArg);i++){
     if(valueArg[i]=='X' || valueArg[i]=='x'){
        places[placesLength] = i;
        placesLength++;
        valueArg[i]='0';
     }
     //if not number throw error?
  }  
  
  boolean exit = false;
  
  while(!exit){
    Serial.println("trying "+String(valueArg));
    
    transmitString(arg,valueArg,lengthArg);
    
    delay(delaySec); 
    
    //increment for next
    boolean inc = true;
    int pos = placesLength;
    while(inc){
        if(valueArg[places[pos]]=='0'){
          valueArg[places[pos]] = '1';
          inc = false; 
        }else{
          valueArg[places[pos]] = '0';         
        }
        pos--;
        if(inc && pos<0){
                   //Serial.println("exit position:"+String(pos));
           exit = true;
           inc=false;
        }

    }
    

  }
  Serial.println("fuzz done");
}

// This gets set as the default handler, and gets called when no other command matches.
void unrecognized(const char *command) {
  Serial.println("usage:");
  Serial.println("  dump (bin | hex | raw)");
  Serial.println("  trans < type > < 0xHEX | Bin > < bit length >");
  Serial.println("  fuzz < type > <binary with 'X's to fuzz> < bit length > < delay in seconds >");
}
