#ifndef __CAMERA_UI_H
#define __CAMERA_UI_H

#include "public.h"

typedef struct lvgl_camera
{
	lv_obj_t *camera;
	lv_obj_t *camera_video;
	lv_obj_t *camera_flash;
	lv_timer_t *camera_flash_timer;
	lv_timer_t *camera_preview_timer;
}lvgl_camera_ui;

extern lv_img_dsc_t photo_show;              //apply an lvgl image variable
extern lvgl_camera_ui guider_camera_ui;      //camera ui structure 

void create_camera_task(uint32_t screen_width, uint32_t screen_height);               //Create camera task thread
void stop_camera_task(void);                 //Close the camera thread
void loopTask_camera(void *pvParameters);    //camera task thread
void ui_set_photo_show(uint32_t screen_width, uint32_t screen_height);

void camera_init(int state);                  //Camera initialization
void setup_scr_camera(lvgl_camera_ui *ui);   //Parameter configuration function on the camera screen

#endif
