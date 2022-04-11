#include <Arduino_MKRIoTCarrier.h>
#include <WiFiNINA.h>
#include <StringSplitter.h>
#include "config.h"
#include "parseUrl.h"
#include <map>

class Event {
  public:
  Event() {};
  Event(unsigned long _end, String _title, String _guests)
    : end(_end), title(_title), guests(_guests) {};
  unsigned long end;
  String title, guests;
};

MKRIoTCarrier carrier;
unsigned long lastUpdated = 0;
unsigned long lastRedrawn = 0;
std::map<unsigned long,Event> events;

void setup() {
  Serial.begin(115200);
  delay(1500);

  // Initialize display
  carrier.begin();
  carrier.withCase();
  showMessage("Connecting to WiFi...");

  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    Serial.println(WiFi.status());
    WiFi.begin(ssid, password);
    Serial.println(WiFi.status());
    delay(3000);
  }
  Serial.print("Connected to "); Serial.println(ssid);
  showMessage("Connected to WiFi!");
}

void loop() {
  unsigned long now = WiFi.getTime();
  if ((now - lastUpdated) >= update_interval)
    getEvents();
  
  if ((now - lastRedrawn) >= 30*1000)
    updateDisplay();
  
  carrier.Buttons.update();
  if (carrier.Button1.onTouchDown()) {
    carrier.Buzzer.beep();
    getEvents();
  }
}

void getEvents() {
  carrier.leds.setPixelColor(1, 0, 0, 0);
  carrier.leds.show();

  WiFiSSLClient client;
  if (!http_req(client, String(url) + String("?room=") + room)) {
    Serial.println("Connection failed!");
    showMessage("Connection failed");
    return;
  }

  events.clear();
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "")
      break;
    StringSplitter spl(line, '|', 4);
    unsigned long start = spl.getItemAtIndex(0).toInt();
    unsigned long end = spl.getItemAtIndex(1).toInt();

    events[start] = Event(end, spl.getItemAtIndex(2), spl.getItemAtIndex(3));
  }

  client.stop();
  lastUpdated = WiFi.getTime();
  carrier.leds.setPixelColor(1, led_brightness, led_brightness, led_brightness);
  carrier.leds.show();
  updateDisplay();
}

void updateDisplay() {
  unsigned long now = WiFi.getTime();
  bool has_events = false;

  for (auto it = events.begin(); it != events.end(); ++it) {
    auto& start = it->first;
    auto& end = it->second.end;
    auto& title = it->second.title;
    auto& guests = it->second.guests;

    // Is this event in the past? If so, ignore it.
    if (end <= now)
      continue;

    // Is this event happening now?
    if (start <= now) {
      carrier.display.fillScreen(ST77XX_RED);
    } else {
      carrier.display.fillScreen(ST77XX_GREEN);
    }
    
    // Show event details
    carrier.display.setTextColor(ST77XX_BLACK);
    carrier.display.setTextSize(3);
    carrier.display.setCursor(10, 90);
    carrier.display.print(title);
    carrier.display.setTextSize(2);
    carrier.display.setCursor(10, 120);
    carrier.display.print(guests);
    has_events = true;
    break;
  }

  if (!has_events) {
    carrier.display.fillScreen(ST77XX_GREEN);
    carrier.display.setTextColor(ST77XX_BLACK);
    carrier.display.setTextSize(2);
    carrier.display.setCursor(20, 120);
    carrier.display.print("No events today");
  }

  lastRedrawn = now;
}

bool http_req(WiFiSSLClient &client, String url) {
  // Follow redirects
  while (1) {
    showMessage("Connecting to server...");
    //Serial.print("\nConnecting to ");
    //Serial.println(url);
    
    ParsedURL parsed_url;
    parseURL(url, &parsed_url);
    if (!client.connect(parsed_url.host.c_str(), parsed_url.port.toInt())) {
      return false;
    }

    client.print("GET ");
    client.print(url);
    client.println(" HTTP/1.0");
    client.print("Host: ");
    client.println(parsed_url.host);
    client.println("User-Agent: Arduino");
    client.println("Connection: close");
    client.println();

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        return true;
      }
      if (line.startsWith("Location: ")) {
        client.stop();
        url = line;
        url.replace("Location: ", "");
        break;
      }
    }
  }
  return false;
}

void showMessage(const char* msg) {
  carrier.display.fillScreen(ST77XX_BLACK);
  carrier.display.setTextColor(ST77XX_WHITE);
  carrier.display.setTextSize(2);
  carrier.display.setCursor(20, 120);
  carrier.display.print(msg);
}
