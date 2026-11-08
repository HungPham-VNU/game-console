// Retro-Go target for the LCDwiki ES3C28P / ES3N28P 2.8" ESP32-S3 display board (N16R8)
// as used in the Pocket Brick Console (see HARDWARE.md in the project root).
//
//   LCD     ILI9341V 240x320 SPI: SCK 12, MOSI 11, MISO 13, DC 46, CS 10, BL 45, RST = EN
//   SD      SDMMC 1-bit: CLK 38, CMD 40, D0 39
//   I2C     SDA 16, SCL 15 (shared: ES8311 0x18, FT6336G touch 0x38, PCF8574 buttons 0x20)
//   Audio   ES8311 codec + FM8002E amp (AMP_EN IO1, low = on). The codec is NOT driven by
//           Retro-Go yet (M6), so emulators are silent for now.
//   Battery 200K/200K divider on IO9 (ADC1 channel 8)
//
// The console is portrait (screen above the buttons) and the board is mounted upside
// down, so the screen is 240x320 rotated 180 degrees.

// Target definition
#define RG_TARGET_NAME              "ES3C28P"

// Storage: built-in microSD slot, native SDMMC in 1-bit mode
#define RG_STORAGE_ROOT             "/sd"
#define RG_STORAGE_SDMMC_HOST       SDMMC_HOST_SLOT_1
#define RG_STORAGE_SDMMC_SPEED      SDMMC_FREQ_DEFAULT

// GPIO extender: PCF8574 (not PCF8574A, whose 0x38 clashes with the touch controller)
#define RG_I2C_GPIO_DRIVER          5   // 5 = PCF8574
#define RG_I2C_GPIO_ADDR            0x20

// Audio: I2S out to the ES8311 (codec init still missing, see header)
#define RG_AUDIO_USE_INT_DAC        0
#define RG_AUDIO_USE_EXT_DAC        1

// Video
#define RG_SCREEN_DRIVER            0   // ILI9341
#define RG_SCREEN_HOST              SPI2_HOST
#define RG_SCREEN_SPEED             SPI_MASTER_FREQ_40M
#define RG_SCREEN_BACKLIGHT         1
#define RG_SCREEN_WIDTH             240
#define RG_SCREEN_HEIGHT            320
#define RG_SCREEN_ROTATE            0
// MADCTL = (RG_SCREEN_ROTATION << 5) | (BGR ? 0x08 : 0). 6 = MX|MY: portrait, rotated 180 deg
// (verified on the real panel 2026-11-08; 4 = MY alone was mirrored). If the image is mirrored or
// upside down, try the other values 0-7. If red and blue are swapped, flip RG_SCREEN_RGB_BGR.
#define RG_SCREEN_ROTATION          6
#define RG_SCREEN_RGB_BGR           1
#define RG_SCREEN_VISIBLE_AREA      {0, 0, 0, 0}
#define RG_SCREEN_SAFE_AREA         {0, 0, 0, 0}
// Standard ILI9341 power/gamma setup (no 0x36 here: MADCTL comes from RG_SCREEN_ROTATION).
#define RG_SCREEN_INIT()                                                                                         \
    ILI9341_CMD(0xCF, 0x00, 0xc3, 0x30);                                                                         \
    ILI9341_CMD(0xED, 0x64, 0x03, 0x12, 0x81);                                                                   \
    ILI9341_CMD(0xE8, 0x85, 0x00, 0x78);                                                                         \
    ILI9341_CMD(0xCB, 0x39, 0x2c, 0x00, 0x34, 0x02);                                                             \
    ILI9341_CMD(0xF7, 0x20);                                                                                     \
    ILI9341_CMD(0xEA, 0x00, 0x00);                                                                               \
    ILI9341_CMD(0xC0, 0x1B);                 /* Power control   //VRH[5:0] */                                    \
    ILI9341_CMD(0xC1, 0x12);                 /* Power control   //SAP[2:0];BT[3:0] */                            \
    ILI9341_CMD(0xC5, 0x32, 0x3C);           /* VCM control */                                                   \
    ILI9341_CMD(0xC7, 0x91);                 /* VCM control2 */                                                  \
    ILI9341_CMD(0xB1, 0x00, 0x10);           /* Frame Rate Control (1B=70, 1F=61, 10=119) */                     \
    ILI9341_CMD(0xB6, 0x0A, 0xA2);           /* Display Function Control */                                      \
    ILI9341_CMD(0xF6, 0x01, 0x30);                                                                               \
    ILI9341_CMD(0xF2, 0x00);                 /* 3Gamma Function Disable */                                       \
    ILI9341_CMD(0x26, 0x01);                 /* Gamma curve selected */                                          \
    ILI9341_CMD(0xE0, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00); \
    ILI9341_CMD(0xE1, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F);
#define RG_SCREEN_DEINIT() \
    lcd_set_backlight(0);  \
    ILI9341_CMD(0x01);

// SNES is too slow on the S3 at this resolution (same choice as the gmt024-s3 target)
#define RG_DISABLE_SNES             1

// Input: 8 buttons on PCF8574 P0..P7, active low. Same wiring as brick-game (HARDWARE.md §3).
#define RG_GAMEPAD_I2C_MAP {\
    {RG_KEY_UP,     .num = 0, .level = 0},\
    {RG_KEY_DOWN,   .num = 1, .level = 0},\
    {RG_KEY_LEFT,   .num = 2, .level = 0},\
    {RG_KEY_RIGHT,  .num = 3, .level = 0},\
    {RG_KEY_A,      .num = 4, .level = 0},\
    {RG_KEY_B,      .num = 5, .level = 0},\
    {RG_KEY_START,  .num = 6, .level = 0},\
    {RG_KEY_SELECT, .num = 7, .level = 0},\
}
// Eight buttons leave no dedicated MENU/OPTION keys: use combos.
#define RG_GAMEPAD_VIRT_MAP {\
    {RG_KEY_MENU,   .src = RG_KEY_START | RG_KEY_SELECT},\
    {RG_KEY_OPTION, .src = RG_KEY_SELECT | RG_KEY_A},\
}
#define RG_RECOVERY_BTN             RG_KEY_DOWN

// Battery: 200K/200K divider → ADC sees half the cell voltage (raw is in mV)
#define RG_BATTERY_DRIVER           1
#define RG_BATTERY_ADC_UNIT         ADC_UNIT_1
#define RG_BATTERY_ADC_CHANNEL      ADC_CHANNEL_8   // GPIO9
#define RG_BATTERY_CALC_PERCENT(raw) (((raw) * 2.f - 3500.f) / (4200.f - 3500.f) * 100.f)
#define RG_BATTERY_CALC_VOLTAGE(raw) ((raw) * 2.f * 0.001f)

// I2C bus
#define RG_GPIO_I2C_SDA             GPIO_NUM_16
#define RG_GPIO_I2C_SCL             GPIO_NUM_15

// Display
#define RG_GPIO_LCD_MISO            GPIO_NUM_13
#define RG_GPIO_LCD_MOSI            GPIO_NUM_11
#define RG_GPIO_LCD_CLK             GPIO_NUM_12
#define RG_GPIO_LCD_CS              GPIO_NUM_10
#define RG_GPIO_LCD_DC              GPIO_NUM_46
#define RG_GPIO_LCD_BCKL            GPIO_NUM_45
// RST is tied to the chip's EN line: no GPIO

// SD card (SDMMC via GPIO matrix)
#define RG_GPIO_SDSPI_CLK           GPIO_NUM_38
#define RG_GPIO_SDSPI_CMD           GPIO_NUM_40
#define RG_GPIO_SDSPI_D0            GPIO_NUM_39

// I2S to the ES8311: BCLK 5, LRCK 7, DSDIN (our data out) 8. MCLK (IO4) not driven yet.
#define RG_GPIO_SND_I2S_BCK         GPIO_NUM_5
#define RG_GPIO_SND_I2S_WS          GPIO_NUM_7
#define RG_GPIO_SND_I2S_DATA        GPIO_NUM_8
// FM8002E shutdown pin: LOW = amp on, so invert the driver's default (high = on)
#define RG_GPIO_SND_AMP_ENABLE      GPIO_NUM_1
#define RG_GPIO_SND_AMP_ENABLE_INVERT

// Updater
#define RG_UPDATER_ENABLE           0
