#pragma once
#include <Arduino.h>

inline void sendFaviconSvg(WiFiClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: image/svg+xml; charset=utf-8");
  client.println("Cache-Control: max-age=86400");
  client.println("Connection: close");
  client.println();

  // Minimal “weather dot” icon
  client.println(
    "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 64 64'>"
    "<defs><linearGradient id='g' x1='0' y1='0' x2='1' y2='1'>"
    "<stop offset='0' stop-color='#7dd3fc'/>"
    "<stop offset='1' stop-color='#a78bfa'/>"
    "</linearGradient></defs>"
    "<rect width='64' height='64' rx='14' fill='#0b1220'/>"
    "<path d='M22 40c-6 0-10-4-10-9s4-9 10-9c2 0 4 .5 5.5 1.5C30 19 34 16 40 16c8 0 14 6 14 14 0 6-4 10-10 10H22z' fill='url(#g)'/>"
    "<circle cx='24' cy='48' r='3' fill='#60a5fa'/>"
    "<circle cx='36' cy='50' r='3' fill='#60a5fa'/>"
    "<circle cx='46' cy='47' r='3' fill='#60a5fa'/>"
    "</svg>"
  );
}
