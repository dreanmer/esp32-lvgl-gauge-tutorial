#include <Arduino.h>
#include <lvgl.h>
#include <Adafruit_GC9A01A.h>
#include <ui/ui.h>

// Pinos para o XIAO ESP32C6
#define TFT_DC    D6
#define TFT_CS    D5
#define TFT_RST   D4

/*Set to your screen resolution and rotation*/
#define TFT_HOR_RES   240
#define TFT_VER_RES   240
#define TFT_ROTATION  2 // 180 graus

#define DEBUG    1

// Instância do display
Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);

static lv_color_t draw_buf[TFT_HOR_RES * TFT_VER_RES / 4];

#if LV_USE_LOG != 0
void my_print(lv_log_level_t level, const char* buf)
{
    LV_UNUSED(level);
    Serial.println(buf);
    Serial.flush();
}
#endif

void gfx_disp_flush(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.writePixels((uint16_t*)px_map, w * h);
    tft.endWrite();

    lv_display_flush_ready(disp);
}

/*use Arduinos millis() as tick source*/
static uint32_t my_tick(void)
{
    return millis();
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Setup start");

    tft.begin();
    tft.setRotation(TFT_ROTATION);

#if DEBUG != 0
    tft.setRotation(0);
    tft.fillScreen(GC9A01A_BLACK);
    tft.setTextColor(GC9A01A_WHITE);
    tft.setTextSize(2);
    tft.setCursor(20, 100);
    tft.println("GC9A01A OK");
    tft.drawCircle(120, 120, 60, GC9A01A_RED);

    delay(1000);
#endif

    /* initializes lvgl */
    Serial.println("LVGL " + String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch());

    lv_init();
    lv_tick_set_cb(my_tick);

    /* register print function for debugging */
#if LV_USE_LOG != 0
    lv_log_register_print_cb(my_print);
#endif

    /* setup lvgl to work with display driver */
    lv_display_t* disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
    lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, gfx_disp_flush);

#if DEBUG != 0
    lv_obj_t* label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello Arduino, I'm LVGL!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    delay(1000);
#endif

    ui_init();

    Serial.println("Setup done");
}

double fps = 0;
unsigned long last_fps_update = 0;
int frame_count = 0;

double current_fps()
{
    frame_count++;

    unsigned long current_time = millis();
    if (current_time - last_fps_update >= 1000) {
        fps = frame_count * 1000.0 / (current_time - last_fps_update);
        frame_count = 0;
        last_fps_update = current_time;
    }

    return fps;
}

lv_color_t previous_color = lv_color_make(255, 0, 0);

void update_arc_color(const lv_color_t color)
{
    if (!lv_color_eq(color, previous_color)) {
        lv_obj_set_style_arc_color(ui_Arc1, color, LV_PART_INDICATOR);
        previous_color = color;
    }
}

void update_arc(int percent_value)
{
    percent_value = std::max(0, std::min(100, percent_value));

    uint16_t hue = 240 - (percent_value * 240 / 100);
    uint8_t saturation = 100;
    uint8_t value = 100;
    lv_color_t color = lv_color_hsv_to_rgb(hue, saturation, value);

    lv_arc_set_value(ui_Arc1, percent_value);
    update_arc_color(color);
}

void update_label(int percent_value)
{
    char buffer[16];
    sprintf(buffer, "%d%%", percent_value);
    lv_label_set_text(ui_Label1, buffer);
}

void update_fps()
{
    char buffer[16];
    sprintf(buffer, "FPS: %.1f", current_fps());
    lv_label_set_text(ui_Label2, buffer);
}

void loop()
{
    static int percent_val = 0;
    static unsigned long last_update = 0;
    static unsigned long interval = random(1000, 3000);
    if (millis() - last_update >= interval) {
        percent_val = random(0, 101);
        last_update = millis();
        interval = random(1000, 3000);
    }

    update_label(percent_val);
    update_arc(percent_val);
    update_fps();

    lv_timer_handler();
    delay(2);
}
