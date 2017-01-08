//define Libraries


#include <ESP8266WiFi.h>

#include <ESP8266WebServer.h> //HTTP WebServer
#include "PietteTech_DHT.h" //DHT sensor lib

//-------------------------------
// defines for DHT lib
#define DHTTYPE  DHT11           // Sensor type DHT11/21/22/AM2301/AM2302
#define DHTPIN   4              // Digital pin for communications
#define REPORT_INTERVAL 10000    // in msec must > 2000
// The port to listen for incoming TCP connections
#define LISTEN_PORT           8080 // WiFI Server listen port

//-------------------------------
//define constants
// WiFi parameters
//const char* ssid = "Delphi_Guests";
//const char* password = "tofave12";

const char* ssid = "SlavaWIFI";
const char* password = "ZXCde3_vgy765_1234567890_vvv";

//Time server 
const char* timeServerHost = "ntp.time.in.ua";
const int timeServerHttpPort = 13;

//const int lightPin = 13;


//-------------------------------
//define globals

// for DHT
unsigned long startMills;
float t, h, d;
int acquireresult;
int acquirestatus;
bool bDHTstarted;       // flag to indicate we started acquisition
int thAverageVar = 0; //divider for get average T and H during period of time
float tempT, tempH, aT, aH = 0;

String dateTime = "";
String webPage = "";
String xml = "";

bool isLightOn = false;
String lightState = "Off";

bool isPowerOn = true;
String powerState = "On";

int maxTranzLevel = 1023; //Maximim level for opening Tranzistor (213 - 4.2V on Gate, 12.0V on Drain) 

//Define pins
int lightPin = 5; //light pin
int powerPin = 13; //power pin


//------------------------------
//Functions declaration

//Libs functions
void dht_wrapper(); // must be declared before the lib initialization

// Init functions
void dhtInit ();
void webServerInit(void);
void WiFiInit();

//User functions
void getTemperature();
String getTime();
//String toString (float data);
String buildWebPage();
void handle_root();
//functions for light control
void handle_lightOn();
void handle_lightOff();
//functions for Power control
void handle_powerOn();
void handle_powerOff();

void handle_xml();
String buildXML();

//------------------------------
// Libs instantiate
PietteTech_DHT DHT(DHTPIN, DHTTYPE, dht_wrapper);
// Create an instance of the server
WiFiServer server(LISTEN_PORT);
//Creating webServer instance with listening of port 80
ESP8266WebServer webServer(80);


//------------------------------
// Initialize the system
void setup()
{

  Serial.begin(115200); 
  //Serial.setDebugOutput(true); // Uncomment if debug serial messages needed
  Serial.println("");
  Serial.println("Setup Started....");
  Serial.println ("-------------------------");
  delay(3000);
  
  //Initializing WiFI
  WiFiInit();
  // init and start Web server
  webServerInit();
  //Initializing DHT
  dhtInit ();

  pinMode(lightPin, OUTPUT); // setting light pin to output
  digitalWrite(lightPin, LOW); //Light is OFF by default

  pinMode(powerPin, OUTPUT); //setting Power pin to output
  digitalWrite(powerPin, HIGH); //Power is ON by default
  

  Serial.println ("Setup finished.");
  Serial.println ("-------------------------");
  Serial.println ("");
}

void loop()
{

	
    webServer.handleClient();
    delay(1);
	
	getTemperature();

	//Serial.println("before client");


	/*Serial.println("response:");
	Serial.println(webServer.args());
	Serial.println(webServer.headers());
	Serial.println(webServer.hostHeader());
	delay(1000);*/
	WiFiClient client = server.available();

	
  if (!client) {
    return;
  }

  //Serial.println("before client available");
  while (!client.available()) {
    delay(1);
  }
  
  
}

void webServerInit(void)
{
	
	webServer.on("/", handle_root); // handling main webpage
	webServer.on("/xml", handle_xml);//handling xml


	webServer.on("/lightOn", handle_lightOn); // light On handling
	webServer.on("/lightOff", handle_lightOff);// light Off handling

	webServer.on("/powerOn", handle_powerOn); // light On handling
	webServer.on("/powerOff", handle_powerOff);// light Off handling

	webServer.begin();
	Serial.println("HTTP server started - port 80");
	Serial.println("");
    Serial.println ("Web server initialization succeed.");
    Serial.println ("");
}

String buildWebPage()
{
	String page = "";

	page += "<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.01 Transitional//EN' 'http://www.w3.org/TR/html4/loose.dtd'>";
	//button and colors styles
	page += "<style>";
	page += ".button {height: 150px; width: 480px; text-align: center; text-decoration: none; font-size: 100px; opacity: 0.8; filter: alpha(opacity=80); border-radius: 30px;} ";
	page += ".blue {background-color: dodgerblue;}";
	page += ".green {background-color: mediumspringgreen}";
	page += ".red {background-color: tomato}";
	page += "</style>";
	
	//Java script
	page += "<SCRIPT>";
	page += "var xmlHttp=createXmlHttpObject();";
	
	page += "function createXmlHttpObject(){ if(window.XMLHttpRequest){ xmlHttp=new XMLHttpRequest();}else{ xmlHttp=new ActiveXObject('Microsoft.XMLHTTP');} return xmlHttp;}";
	
	page += "function process(){";
	page += "if(xmlHttp.readyState==0 || xmlHttp.readyState==4){";
	page += "xmlHttp.open('PUT','xml',true); xmlHttp.onreadystatechange=handleServerResponse; xmlHttp.send(null);}";
	page += "setTimeout('process()',5000);}";

	page += "function handleServerResponse(){";
	page += "if(xmlHttp.readyState==4 && xmlHttp.status==200){";
	page += "xmlResponse=xmlHttp.responseXML;";

	page += "xmldoc = xmlResponse.getElementsByTagName('dateTime'); message = xmldoc[0].firstChild.nodeValue;  document.getElementById('dateTime').innerHTML = message;";
	page += "xmldoc = xmlResponse.getElementsByTagName('temperature'); message = xmldoc[0].firstChild.nodeValue;  document.getElementById('temperature').innerHTML = message;";
	page += "xmldoc = xmlResponse.getElementsByTagName('humidity'); message = xmldoc[0].firstChild.nodeValue;  document.getElementById('humidity').innerHTML = message;";
	page += "xmldoc = xmlResponse.getElementsByTagName('lightState'); message = xmldoc[0].firstChild.nodeValue;  document.getElementById('lightState').innerHTML = message;";
	page += "xmldoc = xmlResponse.getElementsByTagName('powerState'); message = xmldoc[0].firstChild.nodeValue;  document.getElementById('powerState').innerHTML = message;";

	page += "}}";
	
	page += "function sendButton(Parameter){";
	page += "server = '/' + Parameter; ";
	page += "request = new XMLHttpRequest();";
	page += "request.open('GET', server, true);";
	page += "request.send();}";




	page += "</SCRIPT>";


	page += "<html> <head> <title>Test page</title> </head>";
	page += "<body onload='process()'>"; //runnig function when page loaded

	page += "<h1>Test page</h1>";	//page header

	page += "<h2 id='dateTime'> </h2>"; //show date and time
	page += "<h2>Temperature: <span id='temperature'> </span></h2>"; //show temperature
	page += "<h2>Humidity: <span id='humidity'> </span></h2>"; //show humidity
	page += "<h2>Light <span id='lightState'> </span></h2>"; //show light state

	//page += "";
	//page += "<h1> <a href=\"lightOn\"> <button class='button green'>Light On</button> </a>";
	//page += "<a href=\"lightOff\"> <button class='button blue'>Light Off</button> </a> </h1>";
	page += "<h1> <button class='button green' onclick='sendButton(&quot;lightOn&quot;);'>Light On</button>";
	page += "<button class='button blue' onclick='sendButton(&quot;lightOff&quot;);'>Light Off</button> </h1>";

	page += "<h2>Power <span id='powerState'> </span></h2>"; //show power state

	page += "<h1> <button class='button green' onclick='sendButton(&quot;powerOn&quot;);'>Power On</button>";
	page += "<button class='button red' onclick='sendButton(&quot;powerOff&quot;);'>Power Off</button> </h1>";

	//page += "<h1> <a href=\"powerOn\"> <button class='button green'>Power On</button> </a>";
	//page += "<a href=\"powerOff\"> <button class='button red'>Power Off</button> </a> </h1>";
	
	page += "</body> </html>";

	return page;
}

void handle_root()
{
	webPage = buildWebPage();
	webServer.send(200, "text/html", webPage);
	//Serial.println(webPage);
	
}

void handle_xml()
{
	xml = buildXML();
	webServer.send(200, "text/xml", xml);
	Serial.println("");
	Serial.println("xml Sent");
	Serial.println(xml);
	Serial.println("");

}

String buildXML()
{
	String xml = "";

	xml += "<?xml version='1.0'?>";
    xml += "<Donnees>"; 

    xml += "<dateTime>";
	xml += dateTime;
    xml += "</dateTime>";
    
	xml += "<temperature>";
	xml += aT;
	xml += "</temperature>";

	xml += "<humidity>";
	xml += aH;
	xml += "</humidity>";

	xml += "<lightState>";
	xml += lightState;
	xml += "</lightState>";

	xml += "<powerState>";
	xml += powerState;
	xml += "</powerState>";

    xml += "</Donnees>";

	return xml;
}

void handle_lightOn()
{
	if (!isLightOn)
	{
		Serial.println("Switching On light");
		for (int i = 0; i <= maxTranzLevel; i++)
		{
			analogWrite(lightPin, i);
			delay(1);
		}
		
		isLightOn = true;
		lightState = "ON";
		webServer.send(200, "text/html", "OK");
	}
}
void handle_lightOff()
{
	if (isLightOn)
	{
		Serial.println("Switching Off light");
		for (int i = maxTranzLevel; i >= 0; i--)
		{
			analogWrite(lightPin, i);
			delay(2);
		}
		isLightOn = false;
		lightState = "Off";
		webServer.send(200, "text/html", "OK");
	}
}

void handle_powerOn() {
	if (!isPowerOn)
	{
		Serial.println("Switching On Power");
		digitalWrite(powerPin, HIGH);
		isPowerOn = true;
		powerState = "ON";
		webServer.send(200, "text/html", "OK");
	}
}
void handle_powerOff() {
	if (isPowerOn)
	{
		Serial.println("Switching Off Power");
		digitalWrite(powerPin, LOW);
		isPowerOn = false;
		powerState = "Off";
		webServer.send(200, "text/html", "OK");
	}
}


// Get temperature function
void getTemperature(){
  
   if (bDHTstarted) {
    acquirestatus = DHT.acquiring();
    if (!acquirestatus) {
      acquireresult = DHT.getStatus();
      if ( acquireresult == 0 ) {
		  t = DHT.getCelsius();
          h = DHT.getHumidity();
        //d = DHT.getDewPoint();
      }
      bDHTstarted = false;
    }
  }
   //collecting data for getting average T and H
   tempT = tempT + t;
   tempH = tempH + h;
   thAverageVar ++;


  if ((millis() - startMills) > REPORT_INTERVAL) {

	  //getting time
	  getTime();
	 
	  //getting avarage T and H
	  aT = tempT/thAverageVar;
	  aH = tempH/thAverageVar;
	  
	  //converting float to string for Win explorer refresh
	  //saT = toString(aT);
	  //Serial.print("after tostring conversion - ");
	  //Serial.println(saT);
	  //saH = toString(aH);

	  Serial.print("Amounts of measurement: ");
	  Serial.println(thAverageVar);

      Serial.print("Humidity (%): ");
	  Serial.println(aH);


      Serial.print("Temperature (oC): ");
	  Serial.println(aT);
	
	  //setting all tem vars to 0
      startMills = millis();
	  tempT = 0;
	  tempH = 0;
	  thAverageVar = 0;

    // to remove lock
    if (acquirestatus == 1) {
      DHT.reset();
	  thAverageVar = 0;
    }

    if (!bDHTstarted) {
      // non blocking method
      DHT.acquire();
      bDHTstarted = true;
    }
  }

}
// to string function 
String getTime(){
	
	
	WiFiClient client;
	//String datetime = " ";
	Serial.print("");
	Serial.print("connecting to ");
	Serial.println(timeServerHost);
	if (!client.connect(timeServerHost, timeServerHttpPort)) {
		Serial.println("Time server connection failed");

		dateTime = "Time server unavailable";
		return dateTime;
	} else {

		//Serial.println("SEND REQUEST");
		client.print("HEAD / HTTP/1.1\r\nAccept: */*\r\nUser-Agent: Mozilla/4.0 (compatible; ESP8266 NodeMcu Lua;)\r\n\r\n");
	
		delay(100);
	

		while(client.available())
			{
				String line = client.readStringUntil('\r');

				//Parsing date time from server responce
				dateTime = line.substring(0, 11); // getting date
				dateTime += line.substring(20, 25); //getting year
				dateTime += line.substring(10, 20); // getting time
				Serial.println(dateTime);
				return dateTime;
			}
		}

	dateTime = "Time server unavailable";
	return dateTime;
}

void dhtInit ()
{

  startMills = millis();
  
  // delay 2 sec before start acquire DHT
  while ( (millis() - startMills) < 2000 ) {
    yield();
  }

// blocking method for DHT
  acquirestatus = 0;
  acquireresult = DHT.acquireAndWait(1000);
  if ( acquireresult == 0 ) {
    t = DHT.getCelsius();
    h = DHT.getHumidity();
    d = DHT.getDewPoint();
  } else {
    t = h = d = 0;
  }
  Serial.println ("DHT sensor initialization succeed.");
  Serial.println ("");
}

// This wrapper is in charge of calling must be defined like this for the lib work
void dht_wrapper() {
  DHT.isrCallback();
}

//Init wifi
void WiFiInit()
{
	// tryin got connect to WiFi net
  WiFi.mode(WIFI_STA);
  byte tries = 20;
  WiFi.begin(ssid,password);
  
  IPAddress defaultIp(0, 0, 0, 0);
  // checking connection 10 times or untill connection succeed
  while (--tries && WiFi.localIP() == defaultIp)
  {
    Serial.print(".");
	Serial.println(WiFi.localIP());
    delay(1000);
  }

  if (WiFi.localIP() == defaultIp)
  {
    // if unable to connect ot WiFi starting Access Point
    Serial.println("");
    Serial.println("Starting device as Access Point");
    
	WiFi.disconnect();

	const char *ssidAP = "ESP8266AP";
	const char *passwordAP = "12345";

  // changen WiFI mode
  WiFi.mode(WIFI_AP);
 /* IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("ESP8266_AP", "12345");
*/
	WiFi.softAP(ssidAP, passwordAP);
	IPAddress myIP = WiFi.softAPIP();

	Serial.print("WiFi running as Access Point: ");
	Serial.println(ssidAP);
	Serial.print("Password: ");
	Serial.println(passwordAP);
	Serial.print("IP address: ");
    Serial.println(myIP);

  }
  else {
    // ����� ������� ������������ ���������� ���������
    // � ����������� � ������� ����� IP
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  // Start WiFI server
  server.begin();
  Serial.println("WiFi Server started - port 8080");

  Serial.println ("WiFI initialization succeed.");
  Serial.println ("");
}






//toString function
/*
String toString (float data)
{
	char charVal[10];               //temporarily holds data from vals 
	String stringVal = "";     //data on buff is copied to this string
  
	
  dtostrf(data, 4, 2, charVal);  //4 is mininum width, 4 is precision; float value is copied onto buff
  
  //display character array
  Serial.print("charVal: ");
  for(int i=0;i<sizeof(charVal);i++)
  {
    Serial.print(charVal[i]);
  }
  Serial.println();
  //convert chararray to string
  for(int i=0;i<sizeof(charVal);i++)
  {
    stringVal+=charVal[i];
  }
  Serial.print("stringVal: ");Serial.println(stringVal); //display string

  return stringVal;
}
*/