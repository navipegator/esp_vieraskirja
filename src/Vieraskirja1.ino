/*
 * Copyright (c) 2017, Salo Hacklab
 * All rights reserved.
 * Author: Petri Kultanen
 */

/* Simppeli vieraskirja palvelin. */
/* Yhdistä tähän yhteys pisteeseen ja avaa laiteellasi sivu http://www.salohack.fi in a web browser
 */

 #include <ESP8266WiFi.h>
 #include <WiFiClient.h>
 #include <ESP8266WebServer.h>
 #include <DNSServer.h>
 #include <FS.h>

/* Wlan tunnus tähän */
const char * ssid = "SaloHackLab";
//domain name server settings
const byte DNS_PORT = 53;
DNSServer dnsServer;
//set device ip
IPAddress apIP(192, 168, 1, 1);
//start server
ESP8266WebServer server(80);

// hae tidostonimi lista kaikista kommenteista
void getEntrys() {
  //Kommentit on tallennettu /comments hakemistoon
  Dir dir = SPIFFS.openDir("/comments");
  //Muodostetaan json array kaikista komenttien tiedostonimistä
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
  //Lähetetään lista selaimelle json muodossa
  server.send(200, "text/json", output);
}

// hae yksi kommentti
void getEntry() {
  String fileName = "";
  //tarksita että http kutsussa on "?file=jotain" lopussa
  if (server.arg("file") == "") { //Parameter not found
    server.send(400, "text/plain", "filename missing");
  } else {
    //Koita avata tiedosto
    fileName += server.arg("file") + ".json";
    String path = "/comments/" + fileName;
    if (SPIFFS.exists(path)) {
      File file = SPIFFS.open(path, "r");
      //Läheteä tidosto selaimelle
      size_t sent = server.streamFile(file, "text/json");
      file.close();
    } else {
      //Tiedostoa ei löytynyt
      server.send(404, "text/plain", "Not found");
    }
  }
}
// Lisää uusi kommentti
void newEntry() {
  //Selain lähettää kommentit luonti hetken kutsun parametrina "?time=epoch time"
  //Tämä on epoc millisekunteja ja sitä käytetään myös tiedostonimenä
  File f = SPIFFS.open("/comments/" + String(server.arg("time")) + ".json", "w");
  if (!f) {
    //Tiedoston tallennus epäonnistui, levy täynnä tai tiedostonimi virheellinen
    Serial.println("file open failed");
    server.send(500, "text/html", "<h1>Filesystem error</h1>");
  } else {
    //Kirjoita sisältö json tiedostoon
    f.println(server.arg("plain"));
    f.close();
  }
  server.send(200, "text/plain", "OK");
}
// Päivitä index.html tiedosto etänä (OTA), jottei joka muutosta tarvitse laittaa android Iden kautta
void updateIndex() {
  File f = SPIFFS.open("/index.html", "w");
  if (!f) {
    Serial.println("file open failed");
    server.send(500, "text/html", "<h1>Filesystem error</h1>");
  } else {
    f.println(server.arg("plain"));
    f.close();
  }
  server.send(200, "text/plain", "OK");
}
void approveEntry() {
  // todo
}
void removeEntry() {
  // todo
}
void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.print("Conf. access point...");
  //Laitetaan wifi acess point tilaan
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  /* Salasanaa ei tarvita koska verkon halutaan olevan avoin */
  WiFi.softAP(ssid /*, password*/);

  // domain name server asetukset
  // Aika katkaisu, ja virhe viestit, ja palvelun käynnistys
  dnsServer.setTTL(300);
  dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
  dnsServer.start(DNS_PORT, "www.salohack.fi", apIP);

  //device filesystem
  SPIFFS.begin();

  // javascirpt kirjastot
  server.serveStatic("/library", SPIFFS, "/library");
  //Aloitus sivu
  server.serveStatic("/", SPIFFS, "/index.html");
  // lista kommenteista
  server.on("/list", HTTP_GET, getEntrys);
  // lataa yksi kommentti
  server.on("/entry", HTTP_GET, getEntry);
  //Lisää kommentti
  server.on("/entry", HTTP_POST, newEntry);
  //Todo
  server.on("/entry", HTTP_PUT, approveEntry);
  server.on("/entry", HTTP_DELETE, removeEntry);
  //OTA update index.html sivulle
  server.on("/updateindex", HTTP_POST, updateIndex);
  server.begin();
  Serial.println("HTTP server started");

}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}
