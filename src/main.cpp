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

static lv_color_t draw_buf[TFT_HOR_RES * TFT_VER_RES / 10];

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

int previous_color_state = -1;
void update_arc_color(int percent_value) {
    // Só atualiza a cor se houve mudança
    if (current_color_state != previous_color_state) {
        if (current_color_state == 1) {
            lv_obj_set_style_arc_color(ui_Arc1, lv_color_make(255, 0, 0), LV_PART_INDICATOR);
        } else if (current_color_state == 2) {
            lv_obj_set_style_arc_color(ui_Arc1, lv_color_make(0, 255, 0), LV_PART_INDICATOR);
        } else {
            lv_obj_set_style_arc_color(ui_Arc1, lv_color_make(0, 0, 255), LV_PART_INDICATOR);
        }
        previous_color_state = current_color_state;
    }
}

void update_arc(int percent_value) {

    lv_arc_set_value(ui_Arc1, percent_value);

    int current_color_state;

    if (percent_value < 20) {
        current_color_state = 1; // vermelho
    } else if (percent_value > 80) {
        current_color_state = 2; // verde
    } else {
        current_color_state = 0; // azul (padrão)
    }

    // Atualiza a cor do arco
    update_arc_color(percent_value);
}

void loop()
{
    const int pot_val = analogRead(POT_PIN);
    const int pot_val_average = moving_average(pot_val);
    const int pot_percent_val = map(pot_val_average, 10, 3350, 100, 0);

    char buffer[16];
    sprintf(buffer, "%d%%", pot_percent_val);
    lv_label_set_text(ui_Label1, buffer);

    int current_color_state;
    if (pot_percent_val < 20) {
        current_color_state = 1;
    } else if (pot_percent_val > 80) {
        current_color_state = 2;
    } else {
        current_color_state = 0;
    }

    lv_arc_set_value(ui_Arc1, pot_percent_val);
    if (current_color_state != previous_color_state) {
        if (current_color_state == 1) {
            lv_obj_set_style_arc_color(ui_Arc1, lv_color_make(255, 0, 0), LV_PART_INDICATOR);
        } else if (current_color_state == 2) {
            lv_obj_set_style_arc_color(ui_Arc1, lv_color_make(0, 255, 0), LV_PART_INDICATOR);
        } else {
            lv_obj_set_style_arc_color(ui_Arc1, lv_color_make(0, 0, 255), LV_PART_INDICATOR);
        }
        previous_color_state = current_color_state;
    }

    sprintf(buffer, "FPS: %.1f", current_fps());
    lv_label_set_text(ui_Label2, buffer);

    lv_timer_handler();
    delay(2);
}
