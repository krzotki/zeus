#pragma once

// Blue LED at the gun tip, flickered like an electric arc on button press.
// Driven from one GPIO (TIP_LED_PIN) via PWM (LEDC). For a bright blue LED,
// drive it from 5V through an NPN transistor whose base is this pin; a WS2812
// could replace the output later without touching the call sites.
namespace Tip {
  void begin();
  void strike();   // blocking ~0.5s lightning-arc flicker, then off
  void off();
}
