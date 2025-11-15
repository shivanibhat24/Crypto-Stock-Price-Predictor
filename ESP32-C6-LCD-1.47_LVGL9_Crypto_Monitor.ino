// Tutorial : https://youtu.be/JqQEG0eipic
// Use board "ESP32C6 Dev Module" (last tested on v3.2.0)

#define LV_CONF_INCLUDE_SIMPLE // Use the lv_conf.h included in this project, to configure see https://docs.lvgl.io/master/get-started/platforms/arduino.html

#include <WiFi.h>                   // Included with Espressif ESP32C6 Dev Module
#include <HTTPClient.h>             // Included with Espressif ESP32C6 Dev Module
#include "atomic.h"                 // Included in this project
#include "secrets.h"                // Rename secrets_rename.h to secrets.h and modify it to add your Wifi username and password
#include <lvgl.h>                   // Install "lvgl" with the Library Manager (last tested on v9.2.2)
#include "PINS_ESP32-C6-LCD-1_47.h" // Install "GFX Library for Arduino" with the Library Manager (last tested on v1.5.9)
#include "Arduino.h"

#define SCREEN_BRIGHTNESS 255 // You can adjust the screen brightness by changing this value

// LVGL Display global variables
lv_display_t *disp;

// Widgets LVGL global variables
#define SYMBOL_COUNT 3                                                    // Number of symbol to track live on Binance
#define BINANCE_API "https://api.binance.com/api/v3/ticker/price?symbol=" // API call to binance
static lv_obj_t *chart;
static lv_chart_series_t *dataSeries;
static lv_obj_t *rowSym[3];
static lv_obj_t *rowBar[3];
static lv_obj_t *rowPct[3];
static lv_obj_t *lblFooter;

// One card per symbol
static lv_obj_t *priceBox[SYMBOL_COUNT];
static lv_obj_t *priceTitle[SYMBOL_COUNT];
static lv_obj_t *priceLbl[SYMBOL_COUNT];

// Configure the three symbols you want to track on binance
#define PRICE_RANGE 200         // The range of price for the chart, adjust as needed
#define POINTS_TO_CHART 15      // Number of points on the chart, adjust as needed
#define UPDATE_UI_INTERVAL 1000 // UI update in ms, adjust as needed
static const char *symbols[SYMBOL_COUNT] = {"ETHUSDT", "BTCUSDT", "ADAUSDT"};
static float prices[SYMBOL_COUNT] = {0};
static float openPrices[SYMBOL_COUNT] = {0};
static uint8_t symbolIndexToChart = 0; // The symbol index to chart
static uint32_t maxRange;
static uint32_t minRange;

static uint32_t lastApiMs = 0; // Time of last api call

// Simple HTTP GET – returns body as String or empty on fail
static String httpGET(const char *url, uint32_t timeoutMs = 3000)
{
    HTTPClient http;
    http.setTimeout(timeoutMs);
    if (!http.begin(url))
        return String();
    int code = http.GET();
    String payload;
    if (code == 200)
        payload = http.getString();
    http.end();
    return payload;
}

// Parse Binance JSON – very small, avoid ArduinoJson for flash size
static bool parsePrice(const String &body, float &out)
{
    int idx = body.indexOf("\"price\":\"");
    if (idx < 0)
        return false;
    idx += 9; // skip to first digit
    int end = body.indexOf('"', idx);
    if (end < 0)
        return false;
    out = body.substring(idx, end).toFloat();
    return true;
}

// Fetch the symbols' current prices
static void fetchPrice()
{
    float focusPrice = prices[symbolIndexToChart];
    bool focusUpdate = false;

    for (uint8_t i = 0; i < SYMBOL_COUNT; ++i)
    {
        float fetched = prices[i];
        bool ok = false;

        String url = String(BINANCE_API) + symbols[i];
        Serial.printf("[API] GET %s\n", url.c_str());

        String body = httpGET(url.c_str(), 3000);
        if (!body.isEmpty() && parsePrice(body, fetched))
        {
            Serial.printf("[API] OK  -> %s %.2f\n", symbols[i], fetched);
            if (openPrices[i] == 0)
                openPrices[i] = fetched; // capture session open once
            lastApiMs = millis();
            ok = true;
        }
        else
        {
            Serial.printf("[API] ERR -> %s\n", symbols[i]);
        }

        if (ok)
        {
            prices[i] = fetched;
            if (i == symbolIndexToChart)
            {
                focusPrice = fetched;
                focusUpdate = true;
            }
        }
        lv_timer_handler();
    }
}

// Update the UI
void updateUI()
{
    int32_t p = (int32_t)lroundf(prices[symbolIndexToChart] * 100.0f);
    Serial.printf("Price on chart is %ld\n", p);
    lv_chart_set_next_value(chart, dataSeries, p);

    // Enlarge chart window if the point is outside current range
    if (p < minRange || p > maxRange)
    { // range actually changes
        if (p < minRange)
            minRange = p;
        if (p > maxRange)
            maxRange = p;
        Serial.printf("Chart range auto-set to %ld … %ld\n", minRange, maxRange);
    }

    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, minRange, maxRange);

    // Update the price cards
    for (uint8_t i = 0; i < SYMBOL_COUNT; ++i)
    {
        float pct = (prices[i] - openPrices[i]) / openPrices[i] * 100.0f;

        lv_label_set_text_fmt(priceLbl[i],
                              "%.2f  %c%.2f%%",
                              prices[i],
                              pct >= 0 ? '+' : '-', fabsf(pct));

        lv_obj_set_style_text_color(priceLbl[i],
                                    pct >= 0 ? lv_palette_lighten(LV_PALETTE_GREEN, 4)
                                             : lv_palette_lighten(LV_PALETTE_RED, 3),
                                    0);

        lv_color_t bg = pct >= 0
                            ? lv_color_mix(lv_palette_main(LV_PALETTE_GREEN), lv_color_black(), 127)
                            : lv_color_mix(lv_palette_main(LV_PALETTE_RED), lv_color_black(), 127);
        lv_obj_set_style_bg_color(priceBox[i], bg, 0);

        lv_obj_set_height(priceBox[i], LV_SIZE_CONTENT);
    }

    // Update the footer
    lv_label_set_text_fmt(lblFooter,
                          "RAM %u KB",
                          heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024);
}

// Build the UI
static void buildUI()
{
    lv_obj_clean(lv_scr_act());

    // Chart
    chart = lv_chart_create(lv_scr_act());
    lv_chart_set_point_count(chart, POINTS_TO_CHART);
    lv_obj_set_size(chart, 160, 80);
    lv_obj_align(chart, LV_ALIGN_TOP_MID, 0, 24);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    int32_t p = (int32_t)lroundf(openPrices[symbolIndexToChart] * 100.0f);

    maxRange = p + PRICE_RANGE;
    minRange = p - PRICE_RANGE;
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, minRange, maxRange);

    dataSeries = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);

    static lv_obj_t *chartTitle = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(chartTitle, &lv_font_montserrat_18, 0);
    lv_label_set_text_fmt(chartTitle, "%s Chart", symbols[symbolIndexToChart]);
    lv_obj_set_style_text_color(chartTitle,
                                lv_palette_main(LV_PALETTE_CYAN), // bright cyan (#00BCD4)
                                0);
    lv_obj_align_to(chartTitle, chart, LV_ALIGN_OUT_TOP_LEFT, 0, -4);

    // Stacked price cards
    for (uint8_t i = 0; i < SYMBOL_COUNT; ++i)
    {
        priceBox[i] = lv_obj_create(lv_scr_act());
        lv_obj_set_size(priceBox[i], LV_PCT(100), LV_SIZE_CONTENT);

        if (i == 0)
            lv_obj_align(priceBox[i], LV_ALIGN_TOP_LEFT, 0, 112);
        else
            lv_obj_align_to(priceBox[i], priceBox[i - 1],
                            LV_ALIGN_OUT_BOTTOM_LEFT, 0, 3);

        lv_obj_set_style_radius(priceBox[i], 6, 0);
        lv_obj_set_style_pad_all(priceBox[i], 4, 0);

        // Symbol caption
        priceTitle[i] = lv_label_create(priceBox[i]);
        lv_obj_set_style_text_font(priceTitle[i], &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(priceTitle[i], lv_color_white(), 0);
        lv_label_set_text(priceTitle[i], symbols[i]);
        lv_obj_align(priceTitle[i], LV_ALIGN_TOP_LEFT, 0, 0);

        // Live price just below the caption
        priceLbl[i] = lv_label_create(priceBox[i]);
        lv_obj_set_style_text_font(priceLbl[i], &lv_font_montserrat_16, 0);
        lv_obj_align_to(priceLbl[i], priceTitle[i],
                        LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);
        lv_label_set_text(priceLbl[i], "--");
    }

    // Footer
    lblFooter = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(lblFooter, &lv_font_montserrat_16, 0);
    lv_obj_align(lblFooter, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_label_set_text_fmt(lblFooter, "RAM -- KB");
    lv_obj_set_style_text_color(lblFooter, lv_palette_main(LV_PALETTE_CYAN), 0);
}

// LVGL calls this function to print log information
void my_print(lv_log_level_t level, const char *buf)
{
    LV_UNUSED(level);
    Serial.println(buf);
    Serial.flush();
}

// LVGL calls this function to retrieve elapsed time
uint32_t millis_cb(void)
{
    return millis();
}

// LVGL calls this function when a rendered image needs to copied to the display
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);

    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);

    lv_disp_flush_ready(disp);
}

void setup()
{
    Serial.begin(115200);
    DEV_DEVICE_INIT();
    delay(2000); // For debugging, give time for the board to reconnect to com port

    Serial.println("Arduino_GFX LVGL_Arduino_v9 example ");
    String LVGL_Arduino = String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
    Serial.println(LVGL_Arduino);

    // Init Display
    if (!gfx->begin())
    {
        Serial.println("gfx->begin() failed!");
        while (true)
        {
            /* no need to continue */
        }
    }
    gfx->setRotation(0);
    gfx->fillScreen(RGB565_BLACK);
    setDisplayBrigthness();

    // init LVGL
    lv_init();

    // Set a tick source so that LVGL will know how much time elapsed
    lv_tick_set_cb(millis_cb);

    // register print function for debugging
#if LV_USE_LOG != 0
    lv_log_register_print_cb(my_print);
#endif

    uint32_t screenWidth = gfx->width();
    uint32_t screenHeight = gfx->height();
    uint32_t bufSize = screenWidth * 40;

    lv_color_t *disp_draw_buf = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!disp_draw_buf)
    {
        // remove MALLOC_CAP_INTERNAL flag try again
        disp_draw_buf = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_8BIT);
    }
    if (!disp_draw_buf)
    {
        Serial.println("LVGL disp_draw_buf allocate failed!");
        while (true)
        {
            /* no need to continue */
        }
    }

    disp = lv_display_create(screenWidth, screenHeight);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, disp_draw_buf, NULL, bufSize * 2, LV_DISPLAY_RENDER_MODE_PARTIAL);

    wifiConnectionAndFetchPrice();

    Serial.println("Setup done");
    fetchPrice();
    buildUI();

    // Timer to fetch the prices and update the UI
    lv_timer_create([](lv_timer_t *)
                    { fetchPrice(); updateUI(); }, UPDATE_UI_INTERVAL, nullptr);
}

// Wi-Fi connection & first prices fetched with splash screen
void wifiConnectionAndFetchPrice()
{
    static lv_obj_t *wifiSpinner;
    static lv_obj_t *wifiLabel;
    wifiSpinner = lv_spinner_create(lv_scr_act());
    lv_spinner_set_anim_params(wifiSpinner, 8000, 200);
    lv_obj_set_size(wifiSpinner, 80, 80);
    lv_obj_align(wifiSpinner, LV_ALIGN_CENTER, 0, -20);

    wifiLabel = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(wifiLabel, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(wifiLabel, lv_color_hex3(0x0cf), 0);
    lv_obj_set_width(wifiLabel, 140);
    lv_label_set_long_mode(wifiLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(wifiLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text_fmt(wifiLabel, "Connecting to\n%s", WIFI_SSID);
    lv_obj_align_to(wifiLabel, wifiSpinner, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    Serial.printf("Connecting to %s", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    { // keep trying forever
        // Serial.print('.');
        lv_timer_handler(); // keep the spinner animating
    }
    lv_label_set_text(wifiLabel, "Opening Binance Session");
    fetchPrice();

    Serial.printf(" connected, IP: %s\n",
                  WiFi.localIP().toString().c_str());
    lv_obj_del(wifiSpinner);
    lv_obj_del(wifiLabel);
}
void loop()
{
    lv_task_handler(); /* let LVGL do its GUI work */
    delay(5);
}

// Set the brightness of the display to GFX_BRIGHTNESS
void setDisplayBrigthness()
{
    ledcAttachChannel(GFX_BL, 1000, 8, 1);
    ledcWrite(GFX_BL, SCREEN_BRIGHTNESS);
}