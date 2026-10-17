#include "test_platform.h"
#include "battery_math.h"
#include "gallery_navigation.h"
#include "photo_image.h"
#include "storage_format.h"
#include "diagnostics.h"
#include <cassert>
#include <iostream>

// Checks must also run when the compiler defines NDEBUG.
#undef assert
#define assert(condition) do { if (!(condition)) { std::cerr << "FAIL: " #condition << " at line " << __LINE__ << "\n"; std::abort(); } } while (0)

uint32_t test_millis = 0;
extern "C" uint32_t native_tick() { return test_millis; }
int test_adc_mv = 2880; // 3.90 V through the actual board divider.
bool test_sd_present = true;
std::filesystem::path test_sd_root;
lv_indev_t *indev_keypad;
lvgl_camera_ui guider_camera_ui;
static lv_color_t frame[240*135];
static std::filesystem::path output_dir;
static lv_disp_draw_buf_t draw_buffer;
static lv_color_t draw_pixels[240*135];

std::list<File_Entry> list_dir(const char *folder, uint8_t) {
  std::list<File_Entry> files;
  if (!test_sd_present) return files;
  for (const auto &p : std::filesystem::directory_iterator(test_sd_root / (folder+1)))
    files.push_back({p.path().filename().string(), p.is_directory(), p.is_directory() ? 0 : p.file_size()});
  return files;
}
void setup_scr_camera(lvgl_camera_ui *ui) { camera_build_view(ui); ui_attach_controls(ui->camera,ui->camera); }

static void flush(lv_disp_drv_t *d, const lv_area_t *a, lv_color_t *pixels) {
  for (int y=a->y1; y<=a->y2; ++y)
    for (int x=a->x1; x<=a->x2; ++x) frame[y*240+x] = *pixels++;
  lv_disp_flush_ready(d);
}
static void tick(int ms) {
  for (int n=0; n<ms; n+=10) { test_millis += 10; lv_timer_handler(); }
}
static void snapshot(const char *name) {
  lv_refr_now(nullptr);
  std::ofstream out(output_dir / (std::string(name)+".ppm"), std::ios::binary);
  out << "P6\n240 135\n255\n";
  for (auto c:frame) {
    lv_color32_t color; color.full=lv_color_to32(c);
    char rgb[3] = {char(color.ch.red),char(color.ch.green),char(color.ch.blue)};
    out.write(rgb,3);
  }
}
static void key(uint32_t code) { lv_event_send(guider_picture_ui.picture, LV_EVENT_KEY, &code); tick(220); }
static void gallery() {
  lv_obj_t *old=guider_main_ui.main;
  setup_scr_picture(&guider_picture_ui); lv_scr_load(guider_picture_ui.picture);
  guider_main_ui.main=nullptr; lv_obj_del(old); tick(220);
}
static bool has_label(lv_obj_t *obj, const char *text) {
  if (lv_obj_check_type(obj, &lv_label_class) && std::string(lv_label_get_text(obj))==text) return true;
  for (unsigned i=0;i<lv_obj_get_child_cnt(obj);++i)
    if (has_label(lv_obj_get_child(obj,i),text)) return true;
  return false;
}
static void test_math() {
  char usage[48];
  storage_format(usage,sizeof(usage),1048576,31256000000ULL);assert(std::string(usage)=="1.0M/31.3G");
  storage_format(usage,sizeof(usage),1,31256000000ULL);assert(std::string(usage)=="1K/31.3G");
  storage_format(usage,sizeof(usage),0,31256000000ULL);assert(std::string(usage)=="0K/31.3G");
  storage_format(usage,sizeof(usage),2100000000ULL,31900000000ULL);assert(std::string(usage)=="2.1G/31.9G");
  assert(battery_cell_mv(2600)==3200);
  assert(battery_cell_mv(3000)==4200);
  int prev=-1;
  for(int mv=2000; mv<=4500; ++mv) {
    int p=battery_estimate_percent(mv); assert(p>=0 && p<=100 && p>=prev); prev=p;
  }
  GalleryNavigation n; assert(n.up_exits()); n.down(); assert(!n.grid);
  n.count=11; n.down(); assert(n.grid); assert(!n.move(-1));
  for(int i=0;i<8;++i) assert(n.move(1)); assert(n.page_start()==8);
  assert(!n.up_exits() && !n.grid && n.index==8); assert(n.up_exits());
  n.index=10;assert(!n.move(1));
}
int main(int argc,char **argv) {
  assert(argc==3);test_sd_root=argv[1];output_dir=argv[2];std::filesystem::create_directories(output_dir);
  test_math(); lv_init();
  lv_disp_draw_buf_init(&draw_buffer,draw_pixels,nullptr,240*135);
  static lv_disp_drv_t driver;lv_disp_drv_init(&driver);driver.hor_res=240;driver.ver_res=135;
  driver.draw_buf=&draw_buffer;driver.flush_cb=flush;lv_disp_drv_register(&driver);
  static lv_indev_drv_t input;lv_indev_drv_init(&input);input.type=LV_INDEV_TYPE_KEYPAD;
  input.read_cb=[](lv_indev_drv_t *,lv_indev_data_t *d){d->state=LV_INDEV_STATE_REL;};
  indev_keypad=lv_indev_drv_register(&input);
  battery_status_init();setup_scr_main(&guider_main_ui);tick(1300);
  assert(battery_status_percent()==70);
  assert(has_label(guider_main_ui.main, LV_SYMBOL_SD_CARD " 2.1G/31.9G"));snapshot("home");
  test_adc_mv=0;tick(3300);assert(battery_status_percent()==-1);
  test_adc_mv=2880;tick(2300);assert(battery_status_percent()==70);
  test_adc_mv=3000;tick(2300);assert(battery_status_percent()==-1);
  tick(3500);assert(battery_status_percent()==100);
  test_adc_mv=2880;tick(2300);assert(battery_status_percent()==-1);
  tick(3500);assert(battery_status_percent()==70);
  gallery();assert(!lv_obj_has_flag(guider_picture_ui.picture_show,LV_OBJ_FLAG_HIDDEN));snapshot("single");
  key('D');assert(!lv_obj_has_flag(guider_picture_ui.grid,LV_OBJ_FLAG_HIDDEN));snapshot("grid");
  for(int i=0;i<8;++i) key(LV_KEY_RIGHT);snapshot("grid-page-two");
  assert(std::string(lv_label_get_text(guider_picture_ui.picture_counter)).find("9/")==0);
  key('B');assert(lv_obj_has_flag(guider_picture_ui.grid,LV_OBJ_FLAG_HIDDEN));
  assert(std::string(lv_label_get_text(guider_picture_ui.picture_counter)).find("9/")==0);
  key('B');assert(guider_picture_ui.picture==nullptr);assert(lv_scr_act()==guider_main_ui.main);
  size_t timers_before=0;for(auto *t=lv_timer_get_next(nullptr);t;t=lv_timer_get_next(t))++timers_before;
  auto *initial_group=lv_obj_get_group(guider_main_ui.main_imgbtn_camera);
  (void)initial_group;
  for(int i=0;i<40;++i) { gallery();key('D');key('B');key('B'); }
  size_t timers_after=0;for(auto *t=lv_timer_get_next(nullptr);t;t=lv_timer_get_next(t))++timers_after;
  assert(timers_before==timers_after);
  lv_obj_t *old=guider_main_ui.main;setup_scr_camera(&guider_camera_ui);lv_scr_load(guider_camera_ui.camera);
  guider_main_ui.main=nullptr;lv_obj_del(old);tick(100);snapshot("viewfinder");
  old=guider_camera_ui.camera;setup_scr_main(&guider_main_ui);lv_obj_del(old);
  test_sd_present=false;gallery();snapshot("no-sd");key('B');
  test_sd_present=true;
  std::filesystem::path picture_dir=test_sd_root/"picture", backup=test_sd_root/"backup";
  std::filesystem::rename(picture_dir,backup);std::filesystem::create_directory(picture_dir);
  gallery();snapshot("empty");key('B');
  std::filesystem::remove(picture_dir);std::filesystem::rename(backup,picture_dir);
  PhotoImage photo;
  for(const char *name:{"legacy.bmp","top-down.bmp","padded.bmp","rgb24.bmp","sample.jpg"}) {
    auto path=(test_sd_root/name).string();assert(photo_image_load(path.c_str(),photo,40,40,true));
    assert(photo.image.header.w==40 && photo.image.header.h==40);
    lv_color_t c=reinterpret_cast<lv_color_t *>(photo.pixels)[20*40+20];
    lv_color32_t rgb;rgb.full=lv_color_to32(c);assert(rgb.ch.red>220 && rgb.ch.green<30 && rgb.ch.blue<30);
  }
  assert(photo_image_load((test_sd_root/"legacy.bmp").string().c_str(),photo,135,240,false));
  assert(photo.image.header.w==135 && photo.image.header.h==240);
  assert(!photo_image_load((test_sd_root/"broken.bmp").string().c_str(),photo,40,40,true));
  diagnostics_init();
  auto send_command=[](const char *text) { Serial.input += text; Serial.input += "\n"; diagnostics_poll(); };
  send_command("storage");
  assert(Serial.output.find("STORAGE PHOTOS: folder=/picture files=11")!=std::string::npos);
  assert(Serial.output.find("flash_bytes=16777216")!=std::string::npos);
  send_command("battery start");assert(SD_MMC.exists("/battery-test.enabled"));
  auto size_before=std::filesystem::file_size(test_sd_root/"battery-test.csv");
  tick(5200);diagnostics_poll();
  assert(std::filesystem::file_size(test_sd_root/"battery-test.csv")>size_before);
  diagnostics_init();assert(Serial.output.find("Battery test resumed")!=std::string::npos);
  send_command("battery mark battery_only");send_command("battery dump");
  assert(Serial.output.find("BATTERY_LOG_BEGIN")!=std::string::npos);
  assert(Serial.output.find("battery_only")!=std::string::npos);
  assert(Serial.output.find("boot_id,uptime_ms,adc_mv,cell_mv")!=std::string::npos);
  send_command("battery stop");assert(!SD_MMC.exists("/battery-test.enabled"));
  size_before=std::filesystem::file_size(test_sd_root/"battery-test.csv");
  tick(5200);diagnostics_poll();assert(std::filesystem::file_size(test_sd_root/"battery-test.csv")==size_before);
  // A bounded log cannot fill a camera SD card after an interrupted test.
  send_command("battery start");
  { std::ofstream out(test_sd_root/"battery-test.csv",std::ios::app);out<<std::string(33000,'x'); }
  tick(5200);diagnostics_poll();assert(!SD_MMC.exists("/battery-test.enabled"));
  test_sd_present=false;send_command("battery start");
  assert(Serial.output.find("cannot start: no SD card")!=std::string::npos);test_sd_present=true;
  photo_image_free(photo);
  std::cout << "PASS: navigation, paging, 40 screen cycles, battery range/recovery, BMP/JPEG decoding, missing/empty SD; adaptive storage units and persistent bounded battery logging.\n";
}
