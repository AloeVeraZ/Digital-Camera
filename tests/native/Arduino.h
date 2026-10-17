#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cctype>
using std::size_t;
extern uint32_t test_millis;
extern int test_adc_mv;
inline uint32_t millis() { return test_millis; }
inline void delay(int ms) { test_millis += ms; }
inline void pinMode(int, int) {}
inline void analogReadResolution(int) {}
inline void analogSetPinAttenuation(int, int) {}
inline int analogReadMilliVolts(int) { return test_adc_mv; }
constexpr int INPUT = 0, ADC_11db = 3;
struct TestSerial {
  std::string input, output;
  int available() { return !input.empty(); }
  int read() { int c = static_cast<unsigned char>(input[0]); input.erase(0,1); return c; }
  template<class... T> void printf(const char *format, T... args) {
    char text[1024]; snprintf(text,sizeof(text),format,args...); output += text;
  }
  void println(const char *text) { output += text; output += "\n"; }
  void print(const char *text) { output += text; }
  size_t write(const uint8_t *bytes, size_t count) { output.append(reinterpret_cast<const char *>(bytes),count); return count; }
};
struct TestESP {
  uint32_t getFlashChipSize() { return 16*1024*1024; }
  uint32_t getSketchSize() { return 735968; }
};
inline TestESP ESP;
inline uint32_t esp_random() { static uint32_t boot = 1; return ++boot; }
inline TestSerial Serial;
class String : public std::string {
public:
  using std::string::string;
  String(const std::string &s) : std::string(s) {}
  void toLowerCase() { std::transform(begin(),end(),begin(), [](unsigned char c){return std::tolower(c);}); }
  bool endsWith(const char *s) const { size_t n = strlen(s); return size() >= n && compare(size()-n,n,s)==0; }
  int compareTo(const String &s) const { return compare(s); }
};
