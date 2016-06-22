#include "ESP8266xy.h"

#ifdef UNO
SoftwareSerial mySerial(_DBG_RXPIN_,_DBG_TXPIN_);
#endif

#ifdef PROMINI 
SoftwareSerial mySerial(_DBG_RXPIN_,_DBG_TXPIN_);
#endif


int chlID;		//client id(0-4)


boolean WIFI::begin(void)
{
	boolean result = false;
	_cell.begin(57600);	//The default baud rate of ESP8266 is 115200

#ifdef DEBUG
	DebugSerial.begin(debugBaudRate);		//The default baud rate for debugging is 9600
#endif
	flush_rx_buffer();
	_cell.setTimeout(5000);
	_cell.println("AT+RST");
	DBG("AT+RST\r\n");
	//	DBG(_cell.readString());
	unsigned long start;
	start = millis();
	while (millis()-start<15000) {                            
		result = _cell.find("eady");
		if (result)
			break;
	}
	if (result) {
		DBG("Module is ready\r\n");
		return 1;
	} else {
		DBG("Module has no response\r\n");
		return 0;
	}

}


/*************************************************************************
//Initialize port

mode:	setting operation mode
STA: 	Station
AP:	 	Access Point
AT_STA:	Access Point & Station

chl:	channel number
ecn:	encryption
OPEN          0
WEP           1
WAP_PSK       2
WAP2_PSK      3
WAP_WAP2_PSK  4		

return:
true	-	successfully
false	-	unsuccessfully

 ***************************************************************************/
boolean WIFI::Initialize(char mode, char *ssid, char *pwd, byte chl, byte ecn)
{
	if (mode == STA)
	{	
		bool b = confMode(mode);
		if (!b)
		{
			return false;
		}
		Reset();
		b = confJAP(ssid, pwd);
		if (!b)
		{
			return false;
		}
	}
	else if (mode == AP)
	{
		bool b = confMode(mode);
		if (!b)
		{
			return false;
		}
		Reset();
		confSAP(ssid, pwd, chl, ecn);
	}
	else if (mode == AP_STA)
	{
		bool b = confMode(mode);
		if (!b)
		{
			return false;
		}
		Reset();
		b = confJAP(ssid, pwd);
		if (!b)
		{
			return false;
		}
		
		confSAP(ssid, pwd, chl, ecn);
	}

	return true;
}

/*************************************************************************
//Set up tcp or udp connection

type:	tcp or udp

addr:	ip address

port:	port number

a:	set multiple connection
0 for sigle connection
1 for multiple connection

id:	id number(0-4)

return:
true	-	successfully
false	-	unsuccessfully

 ***************************************************************************/
boolean WIFI::ipConfig(byte type, char *addr, char *port, boolean a, byte id)
{
	boolean result = false;
	if (a == 0 )
	{
		//		confMux(a);
		// since confMux(0) is commented out, remember to add it before you call ipConfig.		
		result = newMux(type, addr, port);
	}
	else if (a == 1)
	{
		confMux(a);
		long timeStart = millis();
		while (1)
		{
			long time0 = millis();
			if (time0 - timeStart > 500)
			{
				break;
			}
		}
		result = newMux(id, type, addr, port);
	}
	return result;
}

/*************************************************************************
//receive message from wifi

buf:	buffer for receiving data

chlID:	<id>(0-4)

return:	size of the buffer


 ***************************************************************************/
int WIFI::ReceiveMessage(char *buf)
{
	//+IPD,<len>:<data>
	//+IPD,<id>,<len>:<data>
	String data = "";
	if (_cell.available()>0)
	{

		unsigned long start;
		start = millis();
		char c0 = _cell.read();
		if (c0 == '+')
		{

			while (millis()-start<500) 
			{
				if (_cell.available()>0)
				{
					char c = _cell.read();
					data += c;
				}
				if (data.indexOf("\nOK")!=-1)
				{
					break;
				}
			}
			//Serial.println(data);
			int sLen = strlen(data.c_str());
			int i,j;
			for (i = 0; i <= sLen; i++)
			{
				if (data[i] == ':')
				{
					break;
				}

			}
			boolean found = false;
			for (j = 4; j <= i; j++)
			{
				if (data[j] == ',')
				{
					found = true;
					break;
				}

			}
			int iSize;
			//DBG(data);
			//DBG("\r\n");
			if(found ==true)
			{
				String _id = data.substring(4, j);
				chlID = _id.toInt();
				String _size = data.substring(j+1, i);
				iSize = _size.toInt();
				//DBG(_size);
				String str = data.substring(i+1, i+1+iSize);
				strcpy(buf, str.c_str());	
				//DBG(str);

			}
			else
			{			
				String _size = data.substring(4, i);
				iSize = _size.toInt();
				//DBG(iSize);
				//DBG("\r\n");
				String str = data.substring(i+1, i+1+iSize);
				strcpy(buf, str.c_str());
				//DBG(str);
			}
			return iSize;
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////


/*************************************************************************
//reboot the wifi module



 ***************************************************************************/
void WIFI::Reset(void)
{
	flush_rx_buffer();
	_cell.println("AT+RST");
	unsigned long start;
	start = millis();
	while (millis()-start<5000) {                            
		if(_cell.find("ready")==true)
		{
			DBG("reboot wifi is OK\r\n");
			break;
		}
	}
}

/*********************************************
 *********************************************
 *********************************************
 WIFI Function Commands
 *********************************************
 *********************************************
 *********************************************
 */

/*************************************************************************
//inquire the current mode of wifi module

return:	string of current mode
Station
AP
AP+Station

 ***************************************************************************/
String WIFI::showMode()
{
	String data;
	flush_rx_buffer();
	_cell.println("AT+CWMODE?");  
	unsigned long start;
	start = millis();
	while (millis()-start<2000) {
		if(_cell.available()>0)
		{
			char a =_cell.read();
			data=data+a;
		}
		if (data.indexOf("OK")!=-1)
		{
			break;
		}
	}
	if(data.indexOf("1")!=-1)
	{
		return "Station";
	}else if(data.indexOf("2")!=-1)
	{
		return "AP";
	}else if(data.indexOf("3")!=-1)
	{
		return "AP+Station";
	}
}



/*************************************************************************
//configure the operation mode

a:	
1	-	Station
2	-	AP
3	-	AP+Station

return:
true	-	successfully
false	-	unsuccessfully

 ***************************************************************************/

bool WIFI::confMode(char a)
{
	flush_rx_buffer();
	_cell.print("AT+CWMODE=");  
	_cell.println(a);
	unsigned long start;
	start = millis();
	while (millis()-start<5000) {                            
		if (_cell.find("OK"))
			return true;
	}
	return false;
}



/*************************************************************************
//configure the transmission mode

a:	
0	-	normal mode	
1	-	unvarnished transmission mode

return:
true	-	successfully
false	-	unsuccessfully

 ***************************************************************************/

bool WIFI::confCIPMode(char a)
{
	flush_rx_buffer();
	_cell.print("AT+CIPMODE=");  
	_cell.println(a);
	unsigned long start;
	start = millis();
	while (millis()-start<1000) {                            
		if (_cell.find("OK"))
			return true;
	}
	return false;
}

/*************************************************************************
//show the list of wifi hotspot

return:	string of wifi information
encryption,SSID,RSSI


 ***************************************************************************/

String WIFI::showAP(void)
{
	String data;
	flush_rx_buffer();
	_cell.print("AT+CWLAP\r\n");  
	delay(1000);
	while(1);
	unsigned long start;
	start = millis();
	while (millis()-start<8000) {
		if(_cell.available()>0)
		{
			char a =_cell.read();
			data=data+a;
		}
		if (data.indexOf("OK")!=-1 || data.indexOf("ERROR")!=-1 )
		{
			break;
		}
	}
	if(data.indexOf("ERROR")!=-1)
	{
		return "ERROR";
	}
	else{
		char head[4] = {0x0D,0x0A};   
		char tail[7] = {0x0D,0x0A,0x0D,0x0A};        
		data.replace("AT+CWLAP","");
		data.replace("OK","");
		data.replace("+CWLAP","WIFI");
		data.replace(tail,"");
		data.replace(head,"");

		return data;
	}
}


/*************************************************************************
//show the name of current wifi access port

return:	string of access port name
AP:<SSID>


 ***************************************************************************/
String WIFI::showJAP(void)
{
	flush_rx_buffer();
	_cell.println("AT+CWJAP?");  
	String data;
	unsigned long start;
	start = millis();
	while (millis()-start<3000) {
		if(_cell.available()>0)
		{
			char a =_cell.read();
			data=data+a;
		}
		if (data.indexOf("OK")!=-1 || data.indexOf("ERROR")!=-1 )
		{
			break;
		}
	}
	char head[4] = {0x0D,0x0A};   
	char tail[7] = {0x0D,0x0A,0x0D,0x0A};        
	data.replace("AT+CWJAP?","");
	data.replace("+CWJAP","AP");
	data.replace("OK","");
	data.replace(tail,"");
	data.replace(head,"");

	return data;
}


/*************************************************************************
//configure the SSID and password of the access port

return:
true	-	successfully
false	-	unsuccessfully


 ***************************************************************************/
boolean WIFI::confJAP(char *ssid , char *pwd)
{

	flush_rx_buffer();
	_cell.print("AT+CWJAP=");
	_cell.print("\"");     //"ssid"
	_cell.print(ssid);
	_cell.print("\"");

	_cell.print(",");

	_cell.print("\"");      //"pwd"
	_cell.print(pwd);
	_cell.println("\"");


	unsigned long start;
	start = millis();
	while (millis()-start<30000) {
		if(_cell.find("OK"))
			return true;
	}
	return false;
}
/*************************************************************************
//quite the access port

return:
true	-	successfully
false	-	unsuccessfully


 ***************************************************************************/

boolean WIFI::quitAP(void)
{
	flush_rx_buffer();
	_cell.println("AT+CWQAP");
	unsigned long start;
	start = millis();
	while (millis()-start<3000) {                            
		if(_cell.find("OK")==true)
		{
			return true;

		}
	}
	return false;

}

/*************************************************************************
//show the parameter of ssid, password, channel, encryption in AP mode

return:
mySAP:<SSID>,<password>,<channel>,<encryption>

 ***************************************************************************/
String WIFI::showSAP()
{
	flush_rx_buffer();
	_cell.println("AT+CWSAP?");  
	String data;
	unsigned long start;
	start = millis();
	while (millis()-start<3000) {
		if(_cell.available()>0)
		{
			char a =_cell.read();
			data=data+a;
		}
		if (data.indexOf("OK")!=-1 || data.indexOf("ERROR")!=-1 )
		{
			break;
		}
	}
	char head[4] = {0x0D,0x0A};   
	char tail[7] = {0x0D,0x0A,0x0D,0x0A};        
	data.replace("AT+CWSAP?","");
	data.replace("+CWSAP","mySAP");
	data.replace("OK","");
	data.replace(tail,"");
	data.replace(head,"");

	return data;
}

/*************************************************************************
//configure the parameter of ssid, password, channel, encryption in AP mode

return:
true	-	successfully
false	-	unsuccessfully

 ***************************************************************************/

boolean WIFI::confSAP(char *ssid , char *pwd , byte chl , byte ecn)
{
	flush_rx_buffer();
	_cell.print("AT+CWSAP=");  
	_cell.print("\"");     //"ssid"
	_cell.print(ssid);
	_cell.print("\"");

	_cell.print(",");

	_cell.print("\"");      //"pwd"
	_cell.print(pwd);
	_cell.print("\"");

	_cell.print(",");
	_cell.print(String(chl));

	_cell.print(",");
	_cell.println(String(ecn));
	unsigned long start;
	start = millis();
	while (millis()-start<3000) {                            
		if(_cell.find("OK")==true )
		{
			return true;
		}
	}

	return false;

}


/*********************************************
 *********************************************
 *********************************************
 TPC/IP Function Command
 *********************************************
 *********************************************
 *********************************************
 */

/*************************************************************************
//inquire the connection status

return:		a char indicating the connection status
		2: Got IP, 3: Connected, 4: Disconnected
<ID>  0-4
<type>  tcp or udp
<addr>  ip
<port>  port number

 ***************************************************************************/

char WIFI::showStatus(void)
{
	flush_rx_buffer();
	_cell.println("AT+CIPSTATUS");  
	unsigned long start;
	start = millis();
	while (millis()-start<300) {
		if(_cell.available()){
			if (_cell.read() ==':')
				break;
		}
	}
	
	start = millis();
	while(millis()-start<300){
		if(_cell.available()){
			return _cell.read();
		}
	}
	return 0;
}

/*************************************************************************
//show the current connection mode(sigle or multiple)

return:		string of connection mode
0	-	sigle
1	-	multiple

 ***************************************************************************/
String WIFI::showMux(void)
{
	String data;
	flush_rx_buffer();
	_cell.println("AT+CIPMUX?");  

	unsigned long start;
	start = millis();
	while (millis()-start<3000) {
		if(_cell.available()>0)
		{
			char a =_cell.read();
			data=data+a;
		}
		if (data.indexOf("OK")!=-1)
		{
			break;
		}
	}
	char head[4] = {0x0D,0x0A};   
	char tail[7] = {0x0D,0x0A,0x0D,0x0A};        
	data.replace("AT+CIPMUX?","");
	data.replace("+CIPMUX","showMux");
	data.replace("OK","");
	data.replace(tail,"");
	data.replace(head,"");

	return data;
}

/*************************************************************************
//configure the current connection mode(sigle or multiple)

a:		connection mode
0	-	sigle
1	-	multiple

return:
true	-	successfully
false	-	unsuccessfully
 ***************************************************************************/
boolean WIFI::confMux(boolean a)
{
	flush_rx_buffer();
	_cell.print("AT+CIPMUX=");
	_cell.println(a);           
	unsigned long start;
	start = millis();
	while (millis()-start<300) {                            
		if(_cell.find("OK")==true )
		{
			return true;
		}
	}

	return false;
}


/*************************************************************************
//Set up tcp or udp connection	(signle connection mode)

type:	tcp or udp

addr:	ip address

port:	port number


return:
true	-	successfully
false	-	unsuccessfully

 ***************************************************************************/
boolean WIFI::newMux(byte type, char *addr, char *port)
{
	flush_rx_buffer();
	_cell.print("AT+CIPSTART=");
	if(type==TCP)
	{
		_cell.print("\"TCP\"");
	}else
	{
		_cell.print("\"UDP\"");
	}
	_cell.print(",");
	_cell.print("\"");
	_cell.print(addr);
	_cell.print("\"");
	_cell.print(",");
	//    _cell.print("\"");
	_cell.println(port);
	//    _cell.println("\"");

	unsigned long start;
	start = millis();
	while(millis()-start < 5000){	
		if(_cell.find("OK") || _cell.find("ALREADY")){
			return true;
		}
	}
	return false;
}
/*************************************************************************
//Set up tcp or udp connection	(multiple connection mode)

type:	tcp or udp

addr:	ip address

port:	port number

id:	id number(0-4)

return:
true	-	successfully
false	-	unsuccessfully

 ***************************************************************************/
boolean WIFI::newMux( byte id, byte type, char *addr, char *port)

{
	flush_rx_buffer();
	_cell.print("AT+CIPSTART=");
	_cell.print("\"");
	_cell.print(String(id));
	_cell.print("\"");
	if(type>0)
	{
		_cell.print("\"TCP\"");
	}
	else
	{
		_cell.print("\"UDP\"");
	}
	_cell.print(",");
	_cell.print("\"");
	_cell.print(addr);
	_cell.print("\"");
	_cell.print(",");
	//    _cell.print("\"");
	_cell.println(port);
	//    _cell.println("\"");
	String data;
	unsigned long start;
	start = millis();
	while (millis()-start<300) { 
		if(_cell.available()>0)
		{
			char a =_cell.read();
			data=data+a;
		}
		if (data.indexOf("OK")!=-1 || data.indexOf("ALREAY CONNECT")!=-1 )
		{
			return true;
		}
	}
	return false;


}
/*************************************************************************
//send data in sigle connection mode

str:	string of message

return:
true	-	successfully
false	-	unsuccessfully

 ***************************************************************************/
boolean WIFI::Send(char *str)
{
	flush_rx_buffer();
	_cell.print("AT+CIPSEND=");
	//    _cell.print("\"");
	_cell.println(strlen(str));
	//    _cell.println("\"");
	unsigned long start;
	start = millis();
	bool found = false;
	while (millis()-start<1000) {                            
		if(_cell.find(">")==true){
			found = true;
			break;
		}
	}
	if(found){
		flush_rx_buffer();
		_cell.println(str);
	}else{
		return false;
	}
	start = millis();
	while (millis()-start<1000) {                            
/*		if(_cell.available()){
			DebugSerial.print((char)_cell.read());
		}
*/	
		if(_cell.find("SEND OK"))
			return true;
	}
	return false;
}

boolean WIFI::send_and_receive(char *str, char *str_rcvd, int waiting_for_response_timeout_ms)
{
	int len = strlen(str_rcvd);
	flush_rx_buffer();
	_cell.print("AT+CIPSEND=");
	//    _cell.print("\"");
	_cell.println(strlen(str));
	//    _cell.println("\"");
	unsigned long start;
	start = millis();
	bool found;
	while (millis()-start<1000) {                            
		if(_cell.find(">")==true )
		{
			found = true;
			break;
		}
	}
	if(found)
	{
		flush_rx_buffer();
		_cell.print(str);
	}
	else
	{
		closeMux();
		return false;
	}
	start = millis();
	while (millis()-start<1000) {                            
		if(_cell.find("SEND OK")){
			int i=0;
			long int start2=millis();
			while (millis() - start2 < waiting_for_response_timeout_ms){
				if(_cell.available()){
					str_rcvd[i] = _cell.read();
					i++;
				}
			}
			if(i>len)
				i=len-1;
			str_rcvd[i]='\0';
			return true;
		}
	}
	return false;
}

/*************************************************************************
//send data in multiple connection mode

id:		<id>(0-4)

str:	string of message

return:
true	-	successfully
false	-	unsuccessfully

 ***************************************************************************/
boolean WIFI::Send(byte id, char *str)
{
	flush_rx_buffer();
	_cell.print("AT+CIPSEND=");
	_cell.print(String(id));
	_cell.print(",");
	_cell.println(strlen(str));
	unsigned long start;
	start = millis();
	bool found;
	while (millis()-start<1000) {                          
		if(_cell.find(">")==true )
		{
			found = true;
			break;
		}
	}
	if(found)
	{
		flush_rx_buffer();
		_cell.print(str);
	}
	else
	{
		closeMux(id);
		return false;
	}


	String data;
	start = millis();
	while (millis()-start<1000) {
		if(_cell.available()>0)
		{
			char a =_cell.read();
			data=data+a;
		}
		if (data.indexOf("SEND OK")!=-1)
		{
			return true;
		}
	}
	return false;
}

/*************************************************************************
//Close up tcp or udp connection	(sigle connection mode)


 ***************************************************************************/
void WIFI::closeMux(void)
{
	flush_rx_buffer();
	_cell.println("AT+CIPCLOSE");
	String data;
	unsigned long start;
	start = millis();
	while (millis()-start<500) {
		if(_cell.find("OK"))
			break;
	}
}


/*************************************************************************
//Set up tcp or udp connection	(multiple connection mode)

id:	id number(0-4)

 ***************************************************************************/
void WIFI::closeMux(byte id)
{
	flush_rx_buffer();
	_cell.print("AT+CIPCLOSE=");
	_cell.println(String(id));
	String data;
	unsigned long start;
	start = millis();
	while (millis()-start<300) {
		if(_cell.available()>0)
		{
			char a =_cell.read();
			data=data+a;
		}
		if (data.indexOf("OK")!=-1 || data.indexOf("Link is not")!=-1 || data.indexOf("Cant close")!=-1)
		{
			break;
		}
	}

}

/*************************************************************************
//show the current ip address

return:	string of ip address

 ***************************************************************************/
String WIFI::showIP(void)
{
	String data;
	unsigned long start;
	//DBG("AT+CIFSR\r\n");
	for(int a=0; a<3;a++)
	{
		flush_rx_buffer();
		_cell.println("AT+CIFSR");  
		start = millis();
		while (millis()-start<3000) {
			while(_cell.available()>0)
			{
				char a =_cell.read();
				data=data+a;
			}
			if (data.indexOf("AT+CIFSR")!=-1)
			{
				break;
			}
		}
		if(data.indexOf(".") != -1)
		{
			break;
		}
		data = "";
	}
	//DBG(data);
	//DBG("\r\n");
	char head[4] = {0x0D,0x0A};   
	char tail[7] = {0x0D,0x0D,0x0A};        
	data.replace("AT+CIFSR","");
	data.replace(tail,"");
	data.replace(head,"");

	return data;
}

/*************************************************************************
////set the parameter of server

mode:
0	-	close server mode
1	-	open server mode

port:	<port>

return:
true	-	successfully
false	-	unsuccessfully

 ***************************************************************************/

boolean WIFI::confServer(byte mode, char *port)
{
	flush_rx_buffer();
	_cell.print("AT+CIPSERVER=");  
	_cell.print(String(mode));
	_cell.print(",");
	_cell.println(port);

	String data;
	unsigned long start;
	start = millis();
	boolean found = false;
	while (millis()-start<3000) {
		if(_cell.available()>0)
		{
			char a =_cell.read();
			data=data+a;
		}
		if (data.indexOf("OK")!=-1 || data.indexOf("no charge")!=-1)
		{
			found = true;
			break;
		}
	}
	return found;
}

/*************************************************************************
// flush the input buffer
 ***************************************************************************/

void WIFI::flush_rx_buffer(void)
{
	while(_cell.available()){
		char t = _cell.read();
	}
}

