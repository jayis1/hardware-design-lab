/* ============================================
 * i2c.c — WaveForge I2C Driver Implementation
 * STM32H743 I2C1 master mode, 400 kHz
 * ============================================ */

#include "i2c.h"
#include "registers.h"

#define I2C_TIMEOUT_MS  100

/* Wait for I2C flag with timeout */
static int I2C_WaitFlag(uint32_t flag, uint32_t timeout_ms) {
    uint32_t count = 0;
    while (!(I2C1->ISR & flag)) {
        if (count++ > (timeout_ms * 1000)) {
            return -1;  /* Timeout */
        }
    }
    return 0;
}

void I2C1_Init(void) {
    /* Enable I2C1 clock */
    RCC->APB1LENR |= (1 << 5);  /* I2C1EN */

    /* Wait for clock */
    for (volatile int i = 0; i < 1000; i++);

    /* Reset I2C1 */
    RCC->APB1LRSTR |= (1 << 5);
    RCC->APB1LRSTR &= ~(1 << 5);

    for (volatile int i = 0; i < 1000; i++);

    /* Disable I2C before configuration */
    I2C1->CR1 &= ~I2C_CR1_PE;

    /* Configure timing: 400 kHz at 120 MHz APB1
     * PRESC=3, SCLDEL=4, SDADEL=2, SCLH=17, SCLL=23 */
    I2C1->TIMINGR = (3 << 28)   /* PRESC = 3 */
                  | (4 << 20)   /* SCLDEL = 4 */
                  | (2 << 16)   /* SDADEL = 2 */
                  | (17 << 8)   /* SCLH = 17 */
                  | (23 << 0);  /* SCLL = 23 */

    /* No own address, master only */
    I2C1->OAR1 = 0;
    I2C1->OAR2 = 0;

    /* Enable I2C */
    I2C1->CR1 = I2C_CR1_PE;
}

int I2C1_Write(uint8_t addr, uint8_t reg, const uint8_t *data, uint16_t len) {
    uint32_t count = 0;

    /* Clear any pending flags */
    I2C1->ICR = 0x3F38;

    /* Configure transfer: write, addr, register + data */
    uint32_t total_len = 1 + len;  /* reg addr + data */
    I2C1->CR2 = ((addr & 0x7F) << 1)     /* 7-bit address, shifted */
              | ((total_len & 0xFF) << 16)  /* Number of bytes */
              | I2C_CR2_START              /* Generate START */
              | I2C_CR2_AUTOEND;           /* Auto-generate STOP */

    /* Wait for TXIS or TXE */
    while (!(I2C1->ISR & I2C_ISR_TXIS)) {
        if (I2C1->ISR & I2C_ISR_NACKF) {
            I2C1->ICR = I2C_ICR_NACKCF;
            return -1;
        }
        if (count++ > (I2C_TIMEOUT_MS * 10000)) return -1;
    }

    /* Send register address */
    I2C1->TXDR = reg;

    /* Send data bytes */
    for (uint16_t i = 0; i < len; i++) {
        count = 0;
        while (!(I2C1->ISR & I2C_ISR_TXIS)) {
            if (I2C1->ISR & I2C_ISR_NACKF) {
                I2C1->ICR = I2C_ICR_NACKCF;
                return -1;
            }
            if (count++ > (I2C_TIMEOUT_MS * 10000)) return -1;
        }
        I2C1->TXDR = data[i];
    }

    /* Wait for STOP flag */
    count = 0;
    while (!(I2C1->ISR & I2C_ISR_STOPF)) {
        if (count++ > (I2C_TIMEOUT_MS * 10000)) return -1;
    }
    I2C1->ICR = I2C_ICR_STOPCF;

    return 0;
}

int I2C1_Read(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len) {
    uint32_t count = 0;

    /* Phase 1: Write register address */
    I2C1->CR2 = ((addr & 0x7F) << 1)
              | (1 << 16)           /* 1 byte: register address */
              | I2C_CR2_START
              | (1 << 10);          /* Write direction */

    /* Wait for TXIS */
    count = 0;
    while (!(I2C1->ISR & I2C_ISR_TXIS)) {
        if (I2C1->ISR & I2C_ISR_NACKF) {
            I2C1->ICR = I2C_ICR_NACKCF;
            return -1;
        }
        if (count++ > (I2C_TIMEOUT_MS * 10000)) return -1;
    }

    /* Send register address */
    I2C1->TXDR = reg;

    /* Wait for TC (transfer complete) */
    count = 0;
    while (!(I2C1->ISR & I2C_ISR_TC)) {
        if (count++ > (I2C_TIMEOUT_MS * 10000)) return -1;
    }

    /* Phase 2: Read data with repeated START */
    I2C1->CR2 = ((addr & 0x7F) << 1)
              | ((len & 0xFF) << 16)  /* Number of bytes to read */
              | I2C_CR2_START
              | I2C_CR2_RD_WRN        /* Read direction */
              | I2C_CR2_AUTOEND;       /* Auto STOP */

    /* Read data bytes */
    for (uint16_t i = 0; i < len; i++) {
        count = 0;
        while (!(I2C1->ISR & I2C_ISR_RXNE)) {
            if (count++ > (I2C_TIMEOUT_MS * 10000)) return -1;
        }
        data[i] = (uint8_t)(I2C1->RXDR & 0xFF);
    }

    /* Wait for STOP */
    count = 0;
    while (!(I2C1->ISR & I2C_ISR_STOPF)) {
        if (count++ > (I2C_TIMEOUT_MS * 10000)) return -1;
    }
    I2C1->ICR = I2C_ICR_STOPCF;

    return 0;
}

int I2C1_WriteByte(uint8_t addr, uint8_t reg, uint8_t value) {
    return I2C1_Write(addr, reg, &value, 1);
}

int I2C1_ReadByte(uint8_t addr, uint8_t reg, uint8_t *value) {
    return I2C1_Read(addr, reg, value, 1);
}

int I2C1_IsDeviceReady(uint8_t addr, uint32_t retries) {
    for (uint32_t i = 0; i < retries; i++) {
        /* Send START + address, check for ACK */
        I2C1->CR2 = ((addr & 0x7F) << 1)
                  | (0 << 16)        /* 0 bytes (address probe) */
                  | I2C_CR2_START
                  | I2C_CR2_AUTOEND;

        /* Wait for STOP or NACK */
        uint32_t timeout = 0;
        while (!(I2C1->ISR & (I2C_ISR_STOPF | I2C_ISR_NACKF))) {
            if (timeout++ > 100000) break;
        }

        if (I2C1->ISR & I2C_ISR_STOPF) {
            I2C1->ICR = I2C_ICR_STOPCF;
            return 0;  /* Device acknowledged */
        }

        if (I2C1->ISR & I2C_ISR_NACKF) {
            I2C1->ICR = I2C_ICR_NACKCF;
            /* Device not responding, retry */
        }
    }
    return -1;  /* Device not present */
}