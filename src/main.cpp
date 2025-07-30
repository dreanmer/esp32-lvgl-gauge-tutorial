#include <Arduino.h>
#include <lvgl.h>
#include <Adafruit_GC9A01A.h>
#include <ui/ui.h>
#include "button_manager.h"

#define POT_PIN   A0

// Pinos para o XIAO ESP32C6
#define TFT_DC    D1
#define TFT_CS    D2
#define TFT_RST   D3

// Pinos do painel de controle

// #define BT_UP      D2
#define BT_DN      D5
#define BT_LT      D4
#define BT_RT      D9
// #define BT_OK      D3

/*Set to your screen resolution and rotation*/
#define TFT_HOR_RES   240
#define TFT_VER_RES   240
#define TFT_ROTATION  LV_DISPLAY_ROTATION_0

#define DEBUG    0

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

// uint8_t button_pins[] = {BT_UP, BT_DN, BT_LT, BT_RT, BT_OK};
uint8_t button_pins[] = {BT_DN, BT_LT, BT_RT};
ButtonManager button_manager(button_pins, std::size(button_pins));

void setup()
{
    Serial.begin(115200);
    Serial.println("Setup start");

    tft.begin();

    // pinMode(BT_UP, INPUT_PULLUP);
    pinMode(BT_DN, INPUT_PULLUP);
    pinMode(BT_LT, INPUT_PULLUP);
    pinMode(BT_RT, INPUT_PULLUP);
    // pinMode(BT_OK, INPUT_PULLUP);

    button_manager.begin(true);

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

const int num_readings = 5;
int previous_values[num_readings] = {};
int i = 0;

int stabilize_pot_reading(const int new_value, const int hysteresis)
{
    static int last_stable_value = 0;
    static bool first_call = true;

    if (first_call)
    {
        last_stable_value = new_value;
        first_call = false;
        return new_value;
    }

    previous_values[i] = new_value;
    i = (i + 1) % num_readings;

    long sum = 0;
    for (const int val : previous_values)
    {
        sum += val;
    }
    int avg = sum / num_readings;

    if (abs(avg - last_stable_value) > hysteresis)
    {
        last_stable_value = avg;
    }

    return last_stable_value;
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
    percent_value = std::max(0, std::min(100, percent_value));

    int rounded_percent = ((percent_value + 2) / 5) * 5;
    uint16_t hue = 240 - (rounded_percent * 240 / 100);
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
    const int pot_val = analogRead(POT_PIN);
    const int pot_val_average = stabilize_pot_reading(pot_val, 10);
    const int pot_percent_val = constrain(map(pot_val_average, 60, 3300, 100, 0), 0, 100);

    // Verificar todos os botões uma única vez por loop
    // ButtonEvent up_event = button_manager.checkEvent(BT_UP);
    ButtonEvent dn_event = button_manager.checkEvent(BT_DN);
    ButtonEvent lt_event = button_manager.checkEvent(BT_LT);
    ButtonEvent rt_event = button_manager.checkEvent(BT_RT);
    // ButtonEvent ok_event = button_manager.checkEvent(BT_OK, 2000);

    // Handle screen navigation with left and right buttons
    if (lt_event == BUTTON_CLICK) {
        // Navigate to previous screen
        if (lv_screen_active() == ui_Screen2) {
            lv_screen_load(ui_Screen1);
        } else if (lv_screen_active() == ui_Screen3) {
            lv_screen_load(ui_Screen2);
        }
    }

    if (rt_event == BUTTON_CLICK) {
        // Navigate to next screen
        if (lv_screen_active() == ui_Screen1) {
            lv_screen_load(ui_Screen2);
        } else if (lv_screen_active() == ui_Screen2) {
            lv_screen_load(ui_Screen3);
        }
    }

    update_label(pot_percent_val);
    update_arc(pot_percent_val);
    update_fps();

    lv_timer_handler();
    delay(2);
}
