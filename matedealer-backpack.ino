#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>
#include <FS.h>
#include <ESP8266WebServer.h>

#define VERSION 1

HTTPClient http;
ESP8266WebServer httpd(80);

String token = "";

const char * httpDataHeader = R"(<!DOCTYPE html>
<html>
<head>
<title>matedealer</title>
<link rel="stylesheet" href="/style.css">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no" />
</head>
<body>
<div id="top">
	<span id="title">matedealer</span>
	<a href="/">Configuration</a>
	<a href="/debug">Debug</a>
</div>
)";
const char * httpDataStyleCss = R"(
			html {
				font-family:sans-serif;
				background-color:black;
				color: #e0e0e0;
			}
			div {
				background-color: #202020;
			}
			h1,h2,h3,h4,h5 {
				color: #2020e0;
			}
			a {
				color: #50f050;
			}
			form * {
				display:block;
				border: 1px solid #000;
				font-size: 14px;
				color: #fff;
				background: #004;
				padding: 5px;
			}
)";


void spiffsWrite(String path, String contents) {
	File f = SPIFFS.open(path, "w");
	f.println(contents);
	f.close();
}
String spiffsRead(String path) {
	File f = SPIFFS.open(path, "r");
	String x = f.readStringUntil('\n');
	f.close();
	return x;
}


void setup()
{
	Serial.begin(115200);
	Serial.println();
	delay(200);
	Serial.print("matedealer v"); Serial.println(VERSION);

	Serial.print("Mounting disk... ");
	FSInfo fs_info;
	SPIFFS.begin();
	if ( ! SPIFFS.info(fs_info) ) {
		//the FS info was not retrieved correctly. it's probably not formatted
		Serial.print("failed. Formatting disk... ");
		SPIFFS.format();
		Serial.print("done. Mounting disk... ");
		SPIFFS.begin();
		SPIFFS.info(fs_info);
	}
	Serial.print("done. ");
	Serial.print(fs_info.usedBytes); Serial.print("/"); Serial.print(fs_info.totalBytes); Serial.println(" bytes used");
	
	Serial.println("Checking version");
	String lastVerString = spiffsRead("/version");
	if ( lastVerString.toInt() != VERSION ) {
		//we just got upgrayedded or downgrayedded
		Serial.print("We just moved from v"); Serial.println(lastVerString);
		spiffsWrite("/version", String(VERSION));
		Serial.print("Welcome to v"); Serial.println(VERSION);
	}

	Serial.println("Starting wireless.");
	WiFi.hostname("matedealer");
	WiFiManager wifiManager; //Load the Wi-Fi Manager library.
	wifiManager.setTimeout(300); //Give up with the AP if no users gives us configuration in this many secs.
	if(!wifiManager.autoConnect("matedealer Setup")) {
		Serial.println("failed to connect and hit timeout");
		delay(3000);
		ESP.restart();
	}
	Serial.print("WiFi connected: ");
	Serial.println(WiFi.localIP());

	Serial.println("Starting Web server.");
	httpd.on("/style.css", [&](){
		httpd.send(200, "text/css", httpDataStyleCss);
		httpd.client().stop();
	});
	httpd.on("/", [&](){
		httpd.setContentLength(CONTENT_LENGTH_UNKNOWN);
		httpd.send(200, "text/html", httpDataHeader);
		httpd.client().stop();
	});
	// handler for the /update form POST (once file upload finishes)
	httpd.on("/update", HTTP_POST, [&](){
		httpd.sendHeader("Connection", "close");
		httpd.sendHeader("Access-Control-Allow-Origin", "*");
		httpd.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
	},[&](){
		// handler for the file upload, get's the sketch bytes, and writes
		// them through the Update object
		HTTPUpload& upload = httpd.upload();
		if(upload.status == UPLOAD_FILE_START){
			Serial.printf("Starting HTTP update from %s - other functions will be suspended.\r\n", upload.filename.c_str());
			WiFiUDP::stopAll();

			uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
			if(!Update.begin(maxSketchSpace)){//start with max available size
				Update.printError(Serial);
			}
		} else if(upload.status == UPLOAD_FILE_WRITE){
			Serial.print(upload.totalSize); Serial.printf(" bytes written\r");

			if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
				Update.printError(Serial);
			}
		} else if(upload.status == UPLOAD_FILE_END){
			if(Update.end(true)){ //true to set the size to the current progress
				Serial.printf("Update Success: %u\n", upload.totalSize);
				ESP.restart();
			} else {
				Update.printError(Serial);
			}
		} else if(upload.status == UPLOAD_FILE_ABORTED){
			Update.end();
			Serial.println("Update was aborted");
		}
		delay(0);
	});
	httpd.begin();

	Serial.print("Loading token from disk... ");
	if ( SPIFFS.exists("/token") ) {
		token = spiffsRead("/token");
		Serial.println(token);
	} else {
		Serial.println("Not found. Generating...");
		String letters[]= {"a", "b", "c", "d", "e", "f","g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};
		for ( int i = 0; i < 40; i++ ) {
			token = token + letters[random(0, 36)];
		}
		Serial.println("Done generating. Please wait 10 seconds...");
		delay(10000);
		Serial.print("Here it is: ");
		Serial.println(token);
		spiffsWrite("/token", token);
		Serial.println("Written to disk.");
	}

	Serial.println("setting up http client");
	http.setReuse(true);

	Serial.println("Startup complete");
}

void loop()
{
	
	httpd.handleClient();

}




