#pragma once
#define __PUBLIC_H
#define __DRIVER_SDMMC_H
#define __DISPLAY_H
#include "Arduino.h"
#include "lvgl.h"
#include <memory>
#include <fstream>
#include <filesystem>
#include <list>
#include <vector>
constexpr int LCD_RESET_BATTERY_PIN = 20;
#define CAMERA_FOLDER "/picture"
#define PICTURE_FOLDER "/picture"
#define FILE_READ "rb"
#define FILE_WRITE "wb"
#define FILE_APPEND "ab"
constexpr int CARD_NONE = 0;
extern bool test_sd_present;
extern std::filesystem::path test_sd_root;
namespace fs {
class File {
  std::shared_ptr<std::fstream> stream;
public:
  File() = default;
  File(const std::filesystem::path &p, const char *mode) {
    stream = std::make_shared<std::fstream>(p, std::ios::binary |
        (mode[0]=='w' ? std::ios::out | std::ios::trunc :
         mode[0]=='a' ? std::ios::out | std::ios::app : std::ios::in));
  }
  explicit operator bool() const { return stream && stream->is_open(); }
  size_t read(uint8_t *out, size_t n) { stream->read(reinterpret_cast<char *>(out), n); return stream->gcount(); }
  size_t write(const uint8_t *p, size_t n) { stream->write(reinterpret_cast<const char *>(p), n); return *stream ? n : 0; }
  bool seek(size_t n) { stream->clear(); stream->seekg(n); return bool(*stream); }
  size_t position() { return size_t(stream->tellg()); }
  size_t size() { auto pos=stream->tellg(); stream->seekg(0,std::ios::end); auto n=stream->tellg();stream->seekg(pos);return size_t(n); }
  void close() { if (stream) stream->close(); }
};
}
struct TestSD {
  fs::File open(const char *path, const char *mode) {
    if (!test_sd_present) return {};
    return fs::File(test_sd_root / (path[0]=='/' ? path+1 : path), mode);
  }
  bool exists(const char *path) { return test_sd_present && std::filesystem::exists(test_sd_root / (path+1)); }
  bool remove(const char *path) { return test_sd_present && std::filesystem::remove(test_sd_root / (path+1)); }
  int cardType() { return test_sd_present ? 1 : CARD_NONE; }
  uint64_t totalBytes() { return 31900000000ULL; }
  uint64_t usedBytes() { return 2100000000ULL; }
};
inline TestSD SD_MMC;
struct File_Entry { String name; bool isDirectory; size_t size; };
std::list<File_Entry> list_dir(const char *, uint8_t);
extern lv_indev_t *indev_keypad;
#include "ui_theme.h"
#include "battery_status.h"
#include "main_ui.h"
#include "camera_ui.h"
#include "picture_ui.h"
