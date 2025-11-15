![WhatsApp Image 2025-11-15 at 17 18 54_84f77b36](https://github.com/user-attachments/assets/15cfb220-19df-45af-8aec-5cb5e6d1cf61)# ESP32-C6 Crypto Price Tracker

A real-time cryptocurrency price tracker for ESP32-C6 with LCD display, featuring live price updates, percentage changes, and interactive charts using LVGL graphics library.

![ESP32-C6 Crypto Tracker](https://img.shields.io/badge/ESP32-C6-blue) ![LVGL](https://img.shields.io/badge/LVGL-v9.2.2-green) ![Arduino](https://img.shields.io/badge/Arduino-Compatible-teal)

## Features

- 📊 **Live Price Chart** - Real-time line chart tracking your selected cryptocurrency
- 💰 **Multi-Symbol Tracking** - Monitor up to 3 cryptocurrencies simultaneously
- 📈 **Percentage Change** - See gains/losses since session start with color-coded indicators
- 🎨 **Modern UI** - Beautiful LVGL-based interface with dynamic colors
- 🔄 **Auto-Refresh** - Continuous updates from Binance API
- 💾 **Memory Monitor** - Live RAM usage display

## Hardware Requirements

- **ESP32-C6 Dev Module** with LCD display (1.47" circular display)
- Compatible with boards using the `ESP32-C6-LCD-1.47` pinout
- WiFi connection required

## Software Requirements

### Arduino IDE Setup

1. **Board Support**: Install Espressif ESP32C6 Dev Module (tested on v3.2.0)
2. **Required Libraries** (install via Arduino Library Manager):
   - `lvgl` (v9.2.2 or later)
   - `GFX Library for Arduino` (v1.5.9 or later)

### Dependencies Included

The following are included with the ESP32 board package:
- WiFi.h
- HTTPClient.h

## Installation

1. Clone this repository:
   ```bash
   git clone https://github.com/yourusername/esp32-crypto-tracker.git
   ```

2. Rename `secrets_rename.h` to `secrets.h` and add your WiFi credentials:
   ```cpp
   #define WIFI_SSID "YourWiFiSSID"
   #define WIFI_PASSWORD "YourWiFiPassword"
   ```

3. Open the project in Arduino IDE

4. Select **Tools** → **Board** → **ESP32C6 Dev Module**

5. Upload to your ESP32-C6 device

## Configuration

### Tracked Cryptocurrencies

Edit the `symbols` array to track your preferred cryptocurrencies (default: ETH, BTC, ADA):

```cpp
static const char *symbols[SYMBOL_COUNT] = {"ETHUSDT", "BTCUSDT", "ADAUSDT"};
```

### Adjustable Parameters

```cpp
#define SCREEN_BRIGHTNESS 255    // Display brightness (0-255)
#define PRICE_RANGE 200          // Chart Y-axis range
#define POINTS_TO_CHART 15       // Number of data points on chart
#define UPDATE_UI_INTERVAL 1000  // Refresh rate in milliseconds
```

## How It Works

1. **WiFi Connection**: Connects to your WiFi network on startup with animated splash screen
2. **API Polling**: Fetches current prices from Binance API every second (configurable)
3. **Data Processing**: Parses JSON responses and calculates percentage changes
4. **UI Update**: Renders live chart and price cards with color-coded gains/losses
5. **Auto-Scaling**: Chart automatically adjusts Y-axis range as prices fluctuate

## Display Layout

```
┌─────────────────────┐
│   ETHUSDT Chart     │
│  ┌───────────────┐  │
│  │   Line Chart  │  │
│  └───────────────┘  │
│                     │
│ ┌─────────────────┐ │
│ │ ETHUSDT         │ │
│ │ 2345.67 +2.34%  │ │
│ └─────────────────┘ │
│ ┌─────────────────┐ │
│ │ BTCUSDT         │ │
│ │ 45678.90 -1.23% │ │
│ └─────────────────┘ │
│ ┌─────────────────┐ │
│ │ ADAUSDT         │ │
│ │ 0.4567 +5.67%   │ │
│ └─────────────────┘ │
│                     │
│    RAM 156 KB       │
└─────────────────────┘
```

## Troubleshooting

### Build Errors
- Ensure `lv_conf.h` is properly configured for your setup
- Verify all required libraries are installed at correct versions

### WiFi Connection Issues
- Double-check credentials in `secrets.h`
- Ensure 2.4GHz WiFi is available (ESP32-C6 doesn't support 5GHz)

### API Errors
- Check internet connectivity
- Verify Binance API is accessible in your region
- Symbol names must be valid Binance trading pairs (e.g., "BTCUSDT")

### Display Issues
- Adjust `SCREEN_BRIGHTNESS` if display is too dim/bright
- Verify correct board and pin configuration

## API Reference

This project uses the [Binance Public API](https://api.binance.com/api/v3/ticker/price):
- **Endpoint**: `GET /api/v3/ticker/price?symbol={SYMBOL}`
- **Rate Limits**: No authentication required for public endpoints
- **Free to use**: No API key needed

  ### Output
[img.jpg]()

## License

This project is open source and available under the [MIT License](LICENSE).

## Acknowledgments

- Built with [LVGL](https://lvgl.io/) graphics library
- Uses [Binance API](https://binance-docs.github.io/apidocs/) for price data
- Tutorial video: [YouTube Link](https://youtu.be/JqQEG0eipic)

---

⭐ Star this repository if you find it helpful!
