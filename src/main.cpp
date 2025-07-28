#include <Arduino.h>
#include <lvgl.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <ui/ui.h>

#define POT_PIN   A0

// Pinos para o XIAO ESP32C6
#define TFT_DC    D3
#define TFT_CS    D4
#define TFT_RST   D5

/*Set to your screen resolution and rotation*/
#define TFT_HOR_RES   240
#define TFT_VER_RES   240
#define TFT_ROTATION  LV_DISPLAY_ROTATION_0

#define DEBUG    0

// Instância do display
Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);

static lv_color_t draw_buf[TFT_HOR_RES * TFT_VER_RES];

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
    lv_display_set_rotation(disp, TFT_ROTATION);

#if DEBUG != 0
    lv_obj_t* label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello Arduino, I'm LVGL!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    delay(1000);
#endif

    pinMode(POT_PIN, INPUT);

    ui_init();

    Serial.println("Setup done");
}

const int num_readings = 30;
int previous_values[num_readings] = {};
int i = 0;

int moving_average(const int new_value)
{
    previous_values[i] = new_value;
    i = (i + 1) % num_readings;
    int sum = 0;
    for (const int previous_value : previous_values)
    {
        sum += previous_value;
    }

    return sum / num_readings;
}

double fps = 0;
unsigned long last_fps_update = 0;
int frame_count = 0;

double current_fps()
{
    frame_count++;

    unsigned long current_time = millis();
    if (current_time - last_fps_update >= 1000)
    {
        fps = frame_count * 1000.0 / (current_time - last_fps_update);
        frame_count = 0;
        last_fps_update = current_time;
    }

    return fps;
}

lv_color_t previous_color = lv_color_make(255, 0, 0);

void update_arc_color(const lv_color_t color)
{
    if (!lv_color_eq(color, previous_color))
    {
        lv_obj_set_style_arc_color(ui_Arc1, color, LV_PART_INDICATOR);
        previous_color = color;
    }
}

void update_arc(int percent_value)
{
    lv_arc_set_value(ui_Arc1, percent_value);

    lv_color_t color;

    if (percent_value < 20)
    {
        color = lv_color_make(255, 0, 0);
    }
    else if (percent_value > 80)
    {
        color = lv_color_make(0, 255, 0);
    }
    else
    {
        color = lv_color_make(0, 0, 255);
    }

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
    const int pot_val = analogRead(POT_PIN);
    const int pot_val_average = moving_average(pot_val);
    const int pot_percent_val = map(pot_val_average, 10, 3350, 100, 0);

    update_label(pot_percent_val);
    update_arc(pot_percent_val);
    update_fps();

    lv_timer_handler();
    delay(2);
}
