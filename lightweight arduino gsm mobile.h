/*********************************************************************
PROJECT			:	Lightweight Arduino GSM Mobile phone.
WRITTEN BY		:	Hardin Avishek.
CREATED			:	10 May 2017.

At heart the Arduino act as a middleware to interface the GSM module
and the Nextion display to form the complete GSM Mobile System.

https://youtu.be/fjtdWQdrKaE
https://computronicsgeek.wordpress.com/2017/05/10/lightweight-arduino-gsm-mobile-phone/

*********************************************************************/

#include <SoftwareSerial.h>

SoftwareSerial nextionSerial(10, 11); //Rx, Tx
char msisdn[30], ATcomm[30];
String rawMsg, pageNum, msg, pls;

void setup(){
//Initialize HardwareSerial|SoftwareSerial|GSM module|Nextion display. Perform GSM Location Update.
  Serial.begin(9600);nextionSerial.begin(9600);	
  while(!Serial){;}power_on();delay(3000);
}

void loop(){
  while(nextionSerial.available()){rawMsg.concat(char(nextionSerial.read()));}delay(10); //Read the SoftwareSerial.
  if(!nextionSerial.available()){										
    if(rawMsg.length()){
      pageNum = rawMsg[rawMsg.length()-4];								//Read Nextion: get the page number.
	  msg = rawMsg.substring(1, rawMsg.length()-4);						//Read Nextion: get the Raw Msges from Nextion.
	  if((pageNum == "0") && (msg.length() != 0)){querySMS(msg);}		//Read Nextion: page0, Query all SMS from the GSM Buffer.
      if((pageNum == "1") && (msg.length() != 0)){connectCall(msg);}	//Read Nextion: page1, Dial and Call the B-number.
      if((pageNum == "2") && (msg.length() != 0)){releaseCall(msg);}	//Read Nextion: page2, Release the call.
      if((pageNum == "3") && (msg.length() != 0)){sendSMS(msg);}		//Read Nextion: Page3, Get the content typed in page 3 and send SMS.
	  if((pageNum == "5") && (msg.length() != 0)){answerCall(msg);}		//Read Nextion:	page5, Answer Incoming calls.
	  if((pageNum == "6") && (msg.length() != 0)){delReadSMS(msg);}	  	//Read Nextion:	page6, Delete read SMS.
	  if((pageNum == "7") && (msg.length() != 0)){delSMS(msg);}	  		//Read Nextion: page7, Delete all SMS (incl unread) from the GSM Buffer.
      rawMsg="";pageNum="";msg="";
    }
  }
  
  while(Serial.available()){pls=Serial.readString();}					//Read the HardwareSerial.
  if(!Serial.available() && pls.length()){
    if(pls.indexOf("NO CARRIER") != -1){								//Goto to page0, if B-party Hang up.
      String nextionCallStr = "page page0";writeString(nextionCallStr);
	  sendATcommand("AT", "OK", 2000);	  
    }
	if(pls.indexOf("BUSY") != -1){										//Goto to page0, if B-Number rejects incoming calls.
      String nextionCallStr = "page page0";writeString(nextionCallStr);
	  sendATcommand("AT", "OK", 2000);
	}
	if(pls.indexOf("NO ANSWER") != -1){									//Goto to page0, if B-Number does not answer incoming calls.
      String nextionCallStr = "page page0";writeString(nextionCallStr);
	  sendATcommand("AT", "OK", 2000);
	}		
    if(pls.indexOf("+CLIP") != -1){										//Goto to page5, for any incoming calls.
		int msisdnFirstDelim = pls.indexOf("\"");
		int msisdnSeconddDelim = pls.indexOf("\"", msisdnFirstDelim+1);
		String mobNum = pls.substring(msisdnFirstDelim+1, msisdnSeconddDelim);
		String nextionCallStr = "page page5";writeString(nextionCallStr);
		nextionCallStr = "page5.t1.txt=\""+mobNum+"\"";writeString(nextionCallStr);
		nextionCallStr = "page5.t0.txt=\"Incoming Call\"";writeString(nextionCallStr);
		nextionCallStr = "page5.p0.pic=20";writeString(nextionCallStr);
		sendATcommand("AT", "OK", 2000);
    }
	if(pls.indexOf("+CMTI") != -1){										//If new Msges|Query Msges|Display star picture.
		smsComputation();
		String nextionCallStr = "vis p2,1";writeString(nextionCallStr);
	}pls="";
  }
}

void power_on(){	
//Initialize GSM module, perform Location Update, latch to cellular network, read GSM SMS buffer.
  uint8_t answer = 0;
  while(answer == 0){answer = sendATcommand("AT", "OK", 2000);}
  while((sendATcommand("AT+CREG?", "+CREG: 0,1", 500) || sendATcommand("AT+CREG?", "+CREG: 0,5", 500)) == 0);
  writeString("page0.t0.txt=\"Searching...\"");delay(2000);searchNetwork(); delay(2000);smsComputation(); 
}

int8_t sendATcommand(char* ATcommand, char* expected_answer, unsigned int timeout){
//Function to send AT commands and read responses to/from the GSM module.
  uint8_t x=0, answer=0; char GSMreponse[100]; unsigned long previous;
  memset(GSMreponse, '\0', 100); delay(100);
  while(Serial.available()>0) Serial.read();
  Serial.println(ATcommand); previous = millis();
  do{if(Serial.available() != 0){GSMreponse[x] = Serial.read(); x++;
      if(strstr(GSMreponse, expected_answer) != NULL){answer = 1;}
    }}while((answer == 0) &&((millis() - previous) < timeout));
  return answer;
}

void searchNetwork(){
//Function to search for available cellular networks|Get network registration|Send attched network to Nextion display.
	String sendCOPScommand, readCOPScommad, networkOperator, sendNetOperator;
	int startCOPS=-1, copsFirstcomma, copsSecondcomma, copsNL;
	while(Serial.available())(Serial.read());
	sendCOPScommand="AT+COPS?"; Serial.println(sendCOPScommand);
	while(!Serial.available()); while(Serial.available()){readCOPScommad=Serial.readString();}
	if(readCOPScommad.indexOf("+COPS:") != -1){
		copsFirstcomma=readCOPScommad.indexOf(",", startCOPS+1);
		copsSecondcomma=readCOPScommad.indexOf(",", copsFirstcomma+2);
		copsNL=readCOPScommad.indexOf("\n", copsSecondcomma+1);
		networkOperator=readCOPScommad.substring(copsSecondcomma+2, copsNL-2);
		sendNetOperator="page0.t0.txt=\""+networkOperator+"\"";writeString(sendNetOperator);
	}sendCOPScommand=readCOPScommad=networkOperator="";copsFirstcomma=copsNL=0;
}

void querySMS(String querySMSContent){
//Function to query all SMS from the GSM buffer.
	Serial.println(querySMSContent);
}

void connectCall(String conCallContent){
//Function to Initiate Mobile Originated Call To Dial B-Number.
  String nextionCallStr = "page2.t0.txt=\"Connecting\"";writeString(nextionCallStr);
  while((sendATcommand("AT+CREG?", "+CREG: 0,1", 500) || 	//0,1:	User registered, home network.
    sendATcommand("AT+CREG?", "+CREG: 0,5", 500)) == 0);	//0,5:	User registered, roaming.
  conCallContent.toCharArray(msisdn, rawMsg.length());
  sprintf(ATcomm, "ATD%s;", msisdn);sendATcommand(ATcomm, "OK", 10000);
  nextionCallStr = "page2.t0.txt=\"Calling\"";writeString(nextionCallStr);
  memset(msisdn, '\0', 30);memset(ATcomm, '\0', 30);
}

void releaseCall(String relCallContent){
//Function to Hang up a call.
  Serial.println(relCallContent); 
}

void sendSMS(String sendSMSContent){
//Function to get content typed from Nextion|select SMS Message Format|Send SMS Message to B-number.
  int firstDelim = sendSMSContent.indexOf(byte(189));
  int secondDelim = sendSMSContent.indexOf(byte(189), firstDelim+1);
  String smsContent = sendSMSContent.substring(0, firstDelim);
  String phoneNumber = sendSMSContent.substring(firstDelim+1, secondDelim);
  while((sendATcommand("AT+CREG?", "+CREG: 0,1", 500) || sendATcommand("AT+CREG?", "+CREG: 0,5", 500)) == 0);
  if(sendATcommand("AT+CMGF=1", "OK", 2500)){phoneNumber.toCharArray(msisdn, msg.length());sprintf(ATcomm, "AT+CMGS=\"%s\"", msisdn);
    if(sendATcommand(ATcomm, ">", 2000)){Serial.println(smsContent);Serial.write(0x1A);
      if(sendATcommand("", "OK", 20000)){Serial.println("message sent");}
	  else{Serial.println("error sending");}}
	else{Serial.println("not found >");}
  }smsContent=""; phoneNumber="";memset(msisdn, '\0', 30);memset(ATcomm, '\0', 30);
}

void answerCall(String ansCallContent){
//Function to change Nextion text and picture, if Incoming call is accepted.
	Serial.println(ansCallContent);
	String nextionCallStr = "page5.t0.txt=\"Connected\"";writeString(nextionCallStr);
	nextionCallStr = "page5.p0.pic=21";writeString(nextionCallStr);
}

void smsComputation(){
//Function to Read SMS from GSM buffer. Normally, SMS buffer size=20 & select only 10 SMS.
	int startCPMS=-1, CPMS1comma, CPMS2comma, msgCount=0;
	int startCMGR=-1, cmgrIndex, cmgrNLindex, smsNLindex;
	String sendCPMScommand, readCPMScommand, CPMSMsgCount;
	String sendCMGRcommand, readCMGRcommand, inCMGR, inSMS, dspCount;
	sendCPMScommand = "AT+CPMS?"; Serial.println(sendCPMScommand);
	while(!Serial.available()); while(Serial.available()){readCPMScommand=Serial.readString();}
	if(readCPMScommand.indexOf("+CPMS:") != -1){
		CPMS1comma = readCPMScommand.indexOf(",", startCPMS+1);
		CPMS2comma = readCPMScommand.indexOf(",", CPMS1comma+1);
		CPMSMsgCount = readCPMScommand.substring(CPMS1comma+1, CPMS2comma);msgCount = CPMSMsgCount.toInt();
		dspCount="page6.t51.txt=\""+CPMSMsgCount+"\"";writeString(dspCount);
		if((msgCount > 0) && (msgCount <= 10)){writeString("page6.p1.pic=35");}
		if(msgCount > 10){writeString("page6.p1.pic=34");}int spotSelect = 0;
		while(Serial.available())(Serial.read());
		if(msgCount != 0){
			for(int iSMS=20; iSMS>0; iSMS--){
				sendCMGRcommand="AT+CMGR="+String(iSMS, DEC); Serial.println(sendCMGRcommand);
				while(!Serial.available()); while(Serial.available()){readCMGRcommand=Serial.readString();}
				if((readCMGRcommand.indexOf("+CMGR:")!=-1) && (spotSelect<(50))){
					cmgrIndex = readCMGRcommand.indexOf("+CMGR:", startCMGR+1);
					cmgrNLindex = readCMGRcommand.indexOf("\n", cmgrIndex+1);
					inCMGR = readCMGRcommand.substring(cmgrIndex, cmgrNLindex-1);
					smsNLindex = readCMGRcommand.indexOf("\n", cmgrNLindex+1);
					inSMS = readCMGRcommand.substring(cmgrNLindex+1, smsNLindex-1);
					readSMS(inCMGR, inSMS, iSMS, spotSelect);
					spotSelect=spotSelect+5;
				}startCMGR=-1;cmgrIndex=cmgrNLindex=smsNLindex=0;inCMGR=inSMS=readCMGRcommand="";
			}			
		}
	}readCPMScommand=CPMSMsgCount="";startCPMS=-1;CPMS1comma=CPMS2comma=msgCount=0;
}

void readSMS(String readinCMGR, String readinSMS, int readiSMS ,int readinspotSelect){
//Function to parse the loaded SMS from GSM module and present to Nextion display.
	int startComma=-1, firstColon, firstComma, secondComma, thirdComma;
	String smsSeqNum, smsStatus, smsBNumber, smsDateTime, actualSMS;
	String startOfPage="page6.t", middleOfPage=".txt=\"", pageContent;
	
	firstColon = readinCMGR.indexOf(":", startComma+1);firstComma = readinCMGR.indexOf(",", startComma+1);
	secondComma = readinCMGR.indexOf(",", firstComma+1);thirdComma = readinCMGR.indexOf(",", secondComma+1);smsSeqNum = String(readiSMS);
	smsStatus = readinCMGR.substring(firstColon+7, firstComma-1); smsBNumber = readinCMGR.substring(firstComma+2, secondComma-1);
	smsDateTime = readinCMGR.substring(thirdComma+2, readinCMGR.length()-1);actualSMS = readinSMS.substring(0, readinSMS.length());
	pageContent = startOfPage + readinspotSelect + middleOfPage + smsStatus+"\"";writeString(pageContent);readinspotSelect++;
	pageContent = startOfPage + readinspotSelect + middleOfPage + smsBNumber+"\"";writeString(pageContent);readinspotSelect++;
	pageContent = startOfPage + readinspotSelect + middleOfPage + smsSeqNum+"\"";writeString(pageContent);readinspotSelect++;
	pageContent = startOfPage + readinspotSelect + middleOfPage + smsDateTime+"\"";writeString(pageContent);readinspotSelect++;
	pageContent = startOfPage + readinspotSelect + middleOfPage + actualSMS+"\"";writeString(pageContent);
	smsSeqNum=smsStatus=smsBNumber=smsDateTime=actualSMS=pageContent="";firstColon=firstComma=secondComma=thirdComma=readinspotSelect=0;
}

void delReadSMS(String inDelReadSMS){
//Function to delete only read SMS from the GSM Buffer.
	Serial.println(inDelReadSMS);
}

void delSMS(String indelSMS){
//Function to delete all (Read/Unread) SMS from the GSM Buffer.
	Serial.println(indelSMS);
}

void writeString(String stringData){
//Function to send commands to the Nextion display.
  for(int i=0; i < stringData.length(); i++){nextionSerial.write(stringData[i]);}
  nextionSerial.write(0xff);nextionSerial.write(0xff);nextionSerial.write(0xff);
}

