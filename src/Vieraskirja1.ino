/*
 * Copyright (c) 2017, Salo Hacklab
 * All rights reserved.
 * Author: Petri Kultanen
 */

/* 
   Simple guest book 
   This is example of web server implementing simple rest service to access and save guest book comments.
   We are quite much skipping all security relented issues to keep this example as simple as possible.
   
   Creates access point and hosts web server on it
   Uses Domain name server to give nicer way of opening page on browser

   To use this connect to this acess point and open web page with your browser

   You need to use "ESP8266 Sketch Data upload" tool to send client files to Esp from Data directory
 */
 
 const char * ssid = "MyIoT";
 const char * hostAddress ="www.iot.com";

 #include <ESP8266WiFi.h>
 #include <WiFiClient.h>
 #include <ESP8266WebServer.h>
 #include <DNSServer.h>
 #include <FS.h>

 // Domain name server settings
 const byte DNS_PORT = 53;
 DNSServer dnsServer;
 
 // Set device ip
 IPAddress apIP(192, 168, 1, 1);
 
 //start server @ port 80
 ESP8266WebServer server(80);

// get list of filenames. One comment is one file
void getEntrys() {
  //Comments are in /comments folder
  //Open directory from filesystem
  Dir dir = SPIFFS.openDir("/comments");
  
  //Construct JSON array from all filenames.
  String output = "[";
  while (dir.next()) {
    File entry = dir.openFile("r");
    if (output != "[")
      output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir) ? "dir" : "file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }
  output += "]";
  //Send list to client in JSON
  server.send(200, "text/json", output);
}

// Get one comment
void getEntry() {
  String fileName = "";
  // File name is sent as query parameter, we will check that it exist. (?file=4343)
  if (server.arg("file") == "") {
	// Parameter not found, we will send 400 to client
    server.send(400, "text/plain", "file name missing");
  } else {
    // Trying to open file
    fileName += server.arg("file") + ".json";
    String path = "/comments/" + fileName;
    if (SPIFFS.exists(path)) {
      File file = SPIFFS.open(path, "r");
      // Send file to server
      server.streamFile(file, "text/json");
      file.close();
    } else {
      // File not found send 404 error to client
      server.send(404, "text/plain", "Not found");
    }
  }
}

// Add new comment
void newEntry() {
  // Client will send comment creation time as milliseconds in query parameter "?time=epoch time"
  // This will be used also as filename.
  File f = SPIFFS.open("/comments/" + String(server.arg("time")) + ".json", "w");
  if (!f) {
    // Saving failed, disk full, or malformed file name, we will send 500 error to client
    server.send(500, "text/html", "<h1>Filesystem error</h1>");
  } else {
    // Write omment to disk
    f.println(server.arg("plain"));
    f.close();
  }
  // Send 200 ok to client
  server.send(200, "text/plain", "OK");
}

// This is extra functionality
// You can do OTA update to index.html, so you do not need to flash sketch while developing client side.
// It's very insecure to let this functionality to be accessible without authentication
void updateIndex() {
  File f = SPIFFS.open("/index.html", "w");
  if (!f) {
    Serial.println("file open failed");
    server.send(500, "text/html", "<h1>File system error</h1>");
  } else {
    f.println(server.arg("plain"));
    f.close();
  }
  server.send(200, "text/plain", "OK");
}

void setup() {
  //We will use serial to debug this server
  Serial.begin(115200);
  Serial.println();
  Serial.print("Conf. access point...");
  // Create WiFi access oint
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  /* Password is disabled to keep this network open */
  WiFi.softAP(ssid /*, password*/);

  // DNS server settings
  // Timeout
  dnsServer.setTTL(300);
  //Error handling
  dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
  //Start service
  dnsServer.start(DNS_PORT, hostAddress, apIP);

  // Mount device file system
  SPIFFS.begin();

  //Server to serve static files from file system
  // javascirpt library
  server.serveStatic("/library", SPIFFS, "/library");
  // index page
  server.serveStatic("/", SPIFFS, "/index.html");
  
  // Rest API
  // List of comments
  server.on("/list", HTTP_GET, getEntrys);
  // Get one comment
  server.on("/entry", HTTP_GET, getEntry);
  // Add new comment
  server.on("/entry", HTTP_POST, newEntry);
  // Todo: (Create some approval api to aprove and delete comments
  //server.on("/entry", HTTP_PUT, approveEntry);
  //server.on("/entry", HTTP_DELETE, removeEntry);
  
  // OTA update for index.html 
  server.on("/updateindex", HTTP_POST, updateIndex);
  // Start server
  server.begin();
  Serial.println("HTTP server started");

}

void loop() {
  //Run server
  dnsServer.processNextRequest();
  server.handleClient();
}
