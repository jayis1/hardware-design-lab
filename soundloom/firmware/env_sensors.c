/*
 * env_sensors.c — Environmental sensor drivers
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * SoundLoom bioacoustic soil health monitor.
 * Drivers for:
 *   - Teros-12 soil moisture/EC/temp (SDI-12, ×2 probes)
 *   - GMP343 CO2 NDIR sensor (I2C)
 *   - SHT45 air temperature/humidity (I2C)
 *   - DS18B20 depth thermometers (1-Wire, ×4)
 *   - LSM6DSO32 IMU (SPI)
 *   - Battery voltage / solar voltage (ADC)
 */

#include "env_sensors.h"
#include "board.h"
#include "registers.h"
#include <string.h>
#include <math.h>

/* ---- Private state ---- */
static env_data_t last_reading;
static uint32_t   read_count;

/* ---- Helpers: delay ---- */

static void delay_us(uint32_t us) {
    /* In production: use TIM2 microsecond timer. Simplified here. */
    volatile uint32_t count = us * 48;  /* ~48 cycles/µs at 480 MHz */
    while (count--) __NOP();
}

static void delay_ms(uint32_t ms) {
    HAL_Delay(ms);
}

/* ---- Teros-12 SDI-12 driver ---- */

static UART_HandleTypeDef huart_sdi12;
static uint8_t sdi12_initialized = 0;

static void sdi12_init(void)
{
    if (sdi12_initialized) return;

    __HAL_RCC_UART4_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* TX/RX pins */
    GPIO_InitTypeDef gp = {0};
    gp.Mode = GPIO_MODE_AF_PP;
    gp.Pull = GPIO_PULLUP;
    gp.Speed = GPIO_SPEED_FREQ_LOW;
    gp.Alternate = GPIO_AF6_UART4;
    gp.Pin = SDI12_TX_PIN;
    HAL_GPIO_Init(SDI12_TX_PORT, &gp);
    gp.Pin = SDI12_RX_PIN;
    HAL_GPIO_Init(SDI12_RX_PORT, &gp);

    /* Direction pin */
    gp.Mode = GPIO_MODE_OUTPUT_PP;
    gp.Pull = GPIO_NOPULL;
    gp.Pin = SDI12_DIR_PIN;
    HAL_GPIO_Init(SDI12_DIR_PORT, &gp);
    HAL_GPIO_WritePin(SDI12_DIR_PORT, SDI12_DIR_PIN, GPIO_PIN_RESET);  /* RX mode */

    huart_sdi12.Instance = SDI12_UART;
    huart_sdi12.Init.BaudRate = 1200;  /* SDI-12 is always 1200 baud */
    huart_sdi12.Init.WordLength = UART_WORDLENGTH_8B;
    huart_sdi12.Init.StopBits = UART_STOPBITS_1;
    huart_sdi12.Init.Parity = UART_PARITY_EVEN;
    huart_sdi12.Init.Mode = UART_MODE_TX_RX;
    huart_sdi12.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&huart_sdi12);

    sdi12_initialized = 1;
}

static void sdi12_set_tx(void) {
    HAL_GPIO_WritePin(SDI12_DIR_PORT, SDI12_DIR_PIN, GPIO_PIN_SET);
    delay_us(100);
}
static void sdi12_set_rx(void) {
    HAL_GPIO_WritePin(SDI12_DIR_PORT, SDI12_DIR_PIN, GPIO_PIN_RESET);
    delay_us(100);
}

static void sdi12_send(const char *cmd)
{
    sdi12_set_tx();
    HAL_UART_Transmit(&huart_sdi12, (uint8_t*)cmd, strlen(cmd), 100);
    /* Send break: drive line high then low */
    delay_us(12000);
    sdi12_set_rx();
}

static int sdi12_read_response(char *buf, int maxlen, uint32_t timeout_ms)
{
    int idx = 0;
    uint32_t start = HAL_GetTick();
    while (idx < maxlen - 1 && (HAL_GetTick() - start) < timeout_ms) {
        uint8_t ch;
        if (HAL_UART_Receive(&huart_sdi12, &ch, 1, 100) == HAL_OK) {
            buf[idx++] = (char)ch;
            if (ch == '\n') break;
        }
    }
    buf[idx] = '\0';
    return idx;
}

/* Read Teros-12 on address '0' or '1' (two probes) */
static int teros12_read(uint8_t address, float *moisture, float *temp, float *ec)
{
    char cmd[16];
    char resp[64];

    /* Active measurement: aM! */
    snprintf(cmd, sizeof(cmd), "%cM!", '0' + address);
    sdi12_send(cmd);
    delay_ms(200);  /* Teros-12 needs <1s; typical ~200ms */

    /* Read data: aD0! */
    snprintf(cmd, sizeof(cmd), "%cD0!", '0' + address);
    sdi12_send(cmd);
    int n = sdi12_read_response(resp, sizeof(resp), 1000);
    if (n < 10) return -1;

    /* Parse: a+<moist>+<temp>+<ec>\r\n */
    /* Format: "0+0.1234+23.45+120" */
    float m = 0.0f, t = 0.0f, e = 0.0f;
    if (sscanf(resp, "%*c+%f+%f+%f", &m, &t, &e) == 3) {
        *moisture = m;
        *temp = t;
        *ec = e;
        return 0;
    }
    return -1;
}

/* ---- SHT45 I2C driver ---- */

static I2C_HandleTypeDef hi2c1;

static void i2c1_init(void)
{
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef gp = {0};
    gp.Mode = GPIO_MODE_AF_OD;
    gp.Pull = GPIO_PULLUP;
    gp.Speed = GPIO_SPEED_FREQ_LOW;
    gp.Alternate = GPIO_AF4_I2C1;
    gp.Pin = CO2_SCL_PIN | CO2_SDA_PIN;
    HAL_GPIO_Init(GPIOB, &gp);

    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x30B42C6C;  /* 100 kHz on 120 MHz PCLK */
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

static int sht45_read(float *temp, float *rh)
{
    uint8_t cmd = SHT45_CMD_MEAS_HIGHREP;
    if (HAL_I2C_Master_Transmit(&hi2c1, SHT45_I2C_ADDR << 1, &cmd, 1, 100) != HAL_OK)
        return -1;
    HAL_Delay(10);  /* measurement takes ~8 ms */

    uint8_t data[6];
    if (HAL_I2C_Master_Receive(&hi2c1, SHT45_I2C_ADDR << 1, data, 6, 100) != HAL_OK)
        return -1;

    /* Convert raw to physical values */
    uint16_t raw_t = (data[0] << 8) | data[1];
    uint16_t raw_rh = (data[3] << 8) | data[4];
    *temp = -45.0f + 175.0f * (float)raw_t / 65535.0f;
    *rh = 100.0f * (float)raw_rh / 65535.0f;
    /* Apply temperature compensation for RH */
    *rh = *rh / (1.0f - 0.0015f * (*temp - 25.0f));
    return 0;
}

/* ---- GMP343 CO2 NDIR driver (I2C) ---- */

static int gmp343_read(uint16_t *co2_ppm)
{
    uint8_t cmd = CO2_CMD_START_MEAS;
    if (HAL_I2C_Master_Transmit(&hi2c1, CO2_I2C_ADDR << 1, &cmd, 1, 100) != HAL_OK)
        return -1;
    HAL_Delay(500);  /* NDIR measurement takes ~400ms */

    cmd = CO2_CMD_READ_RESULT;
    if (HAL_I2C_Master_Transmit(&hi2c1, CO2_I2C_ADDR << 1, &cmd, 1, 100) != HAL_OK)
        return -1;
    uint8_t data[2];
    if (HAL_I2C_Master_Receive(&hi2c1, CO2_I2C_ADDR << 1, data, 2, 100) != HAL_OK)
        return -1;
    *co2_ppm = (data[0] << 8) | data[1];
    return 0;
}

/* ---- DS18B20 1-Wire driver ---- */

static uint8_t onewire_reset(void)
{
    /* Pull low for 480 µs, release, wait 70 µs, read */
    GPIO_InitTypeDef gp = {0};
    gp.Pin = ONEWIRE_PIN;
    gp.Mode = GPIO_MODE_OUTPUT_PP;
    gp.Speed = GPIO_SPEED_FREQ_HIGH;
    gp.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(ONEWIRE_PORT, &gp);
    HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, GPIO_PIN_RESET);
    delay_us(480);

    /* Release: switch to input */
    gp.Mode = GPIO_MODE_INPUT;
    gp.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(ONEWIRE_PORT, &gp);
    delay_us(70);

    uint8_t presence = (HAL_GPIO_ReadPin(ONEWIRE_PORT, ONEWIRE_PIN) == GPIO_PIN_RESET) ? 1 : 0;
    delay_us(410);
    return presence;
}

static void onewire_write_bit(uint8_t bit)
{
    GPIO_InitTypeDef gp = {0};
    gp.Pin = ONEWIRE_PIN;
    gp.Mode = GPIO_MODE_OUTPUT_PP;
    gp.Pull = GPIO_NOPULL;
    gp.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(ONEWIRE_PORT, &gp);

    HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, GPIO_PIN_RESET);
    if (bit) {
        delay_us(6);
        gp.Mode = GPIO_MODE_INPUT;
        gp.Pull = GPIO_PULLUP;
        HAL_GPIO_Init(ONEWIRE_PORT, &gp);
        delay_us(64);
    } else {
        delay_us(60);
        gp.Mode = GPIO_MODE_INPUT;
        gp.Pull = GPIO_PULLUP;
        HAL_GPIO_Init(ONEWIRE_PORT, &gp);
        delay_us(10);
    }
}

static uint8_t onewire_read_bit(void)
{
    GPIO_InitTypeDef gp = {0};
    gp.Pin = ONEWIRE_PIN;
    gp.Mode = GPIO_MODE_OUTPUT_PP;
    gp.Pull = GPIO_NOPULL;
    gp.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(ONEWIRE_PORT, &gp);
    HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, GPIO_PIN_RESET);
    delay_us(4);

    gp.Mode = GPIO_MODE_INPUT;
    gp.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(ONEWIRE_PORT, &gp);
    delay_us(8);

    uint8_t bit = (HAL_GPIO_ReadPin(ONEWIRE_PORT, ONEWIRE_PIN) == GPIO_PIN_SET) ? 1 : 0;
    delay_us(50);
    return bit;
}

static void onewire_write_byte(uint8_t byte)
{
    for (int i = 0; i < 8; i++) {
        onewire_write_bit(byte & 0x01);
        byte >>= 1;
    }
}

static uint8_t onewire_read_byte(void)
{
    uint8_t val = 0;
    for (int i = 0; i < 8; i++) {
        val >>= 1;
        if (onewire_read_bit()) val |= 0x80;
    }
    return val;
}

/* Skip ROM (address all devices), convert temperature */
static void ds18b20_start_all(void)
{
    if (!onewire_reset()) return;
    onewire_write_byte(0xCC);  /* Skip ROM */
    onewire_write_byte(0x44);  /* Convert T */
    delay_ms(750);  /* 12-bit conversion: 750 ms max */
}

static int ds18b20_read_sensor(int index, float *temp)
{
    /* In a real multi-sensor bus, we'd use Match ROM with each device's
     * 64-bit ROM address. Here we read from the single bus with skip ROM
     * (only valid for one device). For 4 sensors, a real implementation
     * stores ROM addresses discovered during a bus search.
     * Simplified: read the first device's scratchpad.
     */
    (void)index;  /* would use ROM address in full impl */
    if (!onewire_reset()) return -1;
    onewire_write_byte(0xCC);  /* Skip ROM */
    onewire_write_byte(0xBE);  /* Read scratchpad */

    uint8_t s[9];
    for (int i = 0; i < 9; i++) s[i] = onewire_read_byte();

    int16_t raw = (s[0] | (s[1] << 8));
    *temp = (float)raw / 16.0f;
    return 0;
}

/* ---- Battery & solar voltage ADC ---- */

static ADC_HandleTypeDef hadc1;

static void adc_init(void)
{
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef gp = {0};
    gp.Mode = GPIO_MODE_ANALOG;
    gp.Pull = GPIO_NOPULL;
    gp.Pin = POWER_VBAT_SENSE_PIN | POWER_SOLAR_SENSE_PIN;
    HAL_GPIO_Init(GPIOC, &gp);

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV8;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&hadc1);

    ADC_ChannelConfTypeDef sc = {0};
    sc.Channel = ADC_CHANNEL_1;
    sc.Rank = 1;
    sc.SamplingTime = ADC_SAMPLETIME_247CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sc);
}

static uint16_t adc_read_channel(uint32_t channel)
{
    ADC_ChannelConfTypeDef sc = {0};
    sc.Channel = channel;
    sc.Rank = 1;
    sc.SamplingTime = ADC_SAMPLETIME_247CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sc);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    uint16_t val = (uint16_t)HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return val;
}

/* ---- LSM6DSO32 IMU (SPI3) ---- */

static SPI_HandleTypeDef hspi3;

static void imu_spi_init(void)
{
    __HAL_RCC_SPI3_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gp = {0};
    gp.Mode = GPIO_MODE_AF_PP;
    gp.Pull = GPIO_NOPULL;
    gp.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gp.Alternate = GPIO_AF6_SPI3;
    /* SCK=PC10, MISO=PC11, MOSI=PC12 */
    /* Actually SPI3 on this board uses PA pins per board.h */
    /* For this implementation we use software SPI on IMU_NSS */
    /* Simplified: initialise as GPIO bit-bang */
    gp.Mode = GPIO_MODE_OUTPUT_PP;
    gp.Pin = IMU_NSS_PIN;
    HAL_GPIO_Init(IMU_NSS_PORT, &gp);
    HAL_GPIO_WritePin(IMU_NSS_PORT, IMU_NSS_PIN, GPIO_PIN_SET);
}

static int imu_read_id(void)
{
    /* Simplified: would read LSM_WHO_AM_I register via SPI3.
     * Returns expected 0x6C in production.
     */
    return LSM_WHO_AM_I_VAL;
}

/* ---- Public API ---- */

void env_init(void)
{
    sdi12_init();
    i2c1_init();
    adc_init();
    imu_spi_init();
    memset(&last_reading, 0, sizeof(last_reading));
    read_count = 0;
}

int env_read_all(env_data_t *out)
{
    int errors = 0;

    /* Teros-12 probes (2 ×) */
    float m1, t1, ec1, m2, t2, ec2;
    if (teros12_read(0, &m1, &t1, &ec1) == 0) {
        last_reading.moisture[0] = m1;
        last_reading.temp_teros[0] = t1;
        last_reading.ec[0] = ec1;
    } else errors++;

    if (teros12_read(1, &m2, &t2, &ec2) == 0) {
        last_reading.moisture[1] = m2;
        last_reading.temp_teros[1] = t2;
        last_reading.ec[1] = ec2;
    } else errors++;

    /* SHT45 air temp/RH */
    float air_t, rh;
    if (sht45_read(&air_t, &rh) == 0) {
        last_reading.air_temp = air_t;
        last_reading.air_rh = rh;
    } else errors++;

    /* GMP343 CO2 */
    uint16_t co2;
    if (gmp343_read(&co2) == 0) {
        last_reading.co2_ppm = co2;
    } else errors++;

    /* DS18B20 depth thermometers (4 ×) */
    ds18b20_start_all();
    for (int i = 0; i < DS18B20_COUNT; i++) {
        float t;
        if (ds18b20_read_sensor(i, &t) == 0) {
            last_reading.depth_temp[i] = t;
        } else errors++;
    }

    /* Battery voltage: ADC raw → voltage
     * Vbat = raw / 4095 * 3.3 * (R1+R2)/R2  with divider ratio 3:1
     */
    uint16_t raw_bat = adc_read_channel(ADC_CHANNEL_1);
    last_reading.battery_mv = (uint16_t)((float)raw_bat / 4095.0f * 3.3f * 3.0f * 1000.0f);

    /* Solar voltage on channel 4 */
    uint16_t raw_sol = adc_read_channel(ADC_CHANNEL_4);
    last_reading.solar_mv = (uint16_t)((float)raw_sol / 4095.0f * 3.3f * 3.0f * 1000.0f);

    last_reading.timestamp = HAL_GetTick();
    read_count++;

    if (out) *out = last_reading;
    return errors;
}

const env_data_t* env_get_last(void)
{
    return &last_reading;
}

uint32_t env_get_read_count(void)
{
    return read_count;
}

/* ---- Get battery percentage estimate ---- */

uint8_t env_battery_percent(void)
{
    /* Li-SOCl2: 3.67V full, 3.2V empty (approx) */
    float mv = (float)last_reading.battery_mv;
    float pct = (mv - 3200.0f) / (3670.0f - 3200.0f) * 100.0f;
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 100.0f) pct = 100.0f;
    return (uint8_t)pct;
}