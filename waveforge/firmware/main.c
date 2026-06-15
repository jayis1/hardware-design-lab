/* ============================================
 * main.c — WaveForge Synthesizer Engine
 * STM32H743VIT6, 480 MHz Cortex-M7
 * ============================================ */

#include "board.h"
#include "registers.h"
#include "drivers/i2c.h"
#include "drivers/spi_flash.h"
#include "drivers/wm8778.h"
#include "drivers/tpa6130.h"
#include <string.h>

/* ---- Audio DMA Buffers (in DTCM for zero-wait access) ---- */
#define AUDIO_BUF_SAMPLES 256
#define AUDIO_BUF_CHANNELS 2

/* Double-buffered stereo audio output (left | right packed 32-bit) */
__attribute__((section(".audio_buf"))) __attribute__((aligned(32)))
static volatile int32_t audio_tx_buf[AUDIO_BUF_SAMPLES * AUDIO_BUF_CHANNELS * 2];

/* Double-buffered stereo audio input from codec ADC */
__attribute__((section(".audio_buf"))) __attribute__((aligned(32)))
static volatile int32_t audio_rx_buf[AUDIO_BUF_SAMPLES * AUDIO_BUF_CHANNELS * 2];

/* ---- Voice Allocator State ---- */
#define MAX_VOICES 64
#define MAX_WAVETABLE_SIZE 2048

typedef enum {
    OSC_MODE_WAVETABLE = 0,
    OSC_MODE_FM,
    OSC_MODE_NOISE
} osc_mode_t;

typedef struct {
    uint8_t  note;          /* MIDI note number */
    uint8_t  velocity;      /* MIDI velocity */
    uint8_t  active;        /* 1 = playing, 0 = free */
    uint8_t  channel;       /* MIDI channel */
    uint32_t phase;         /* 32-bit phase accumulator */
    uint32_t phase_inc;     /* Frequency-dependent increment */
    int16_t  wavetable_idx; /* Index into wavetable ROM */
    osc_mode_t osc_mode;   /* Oscillator mode */
    int32_t  fm_depth;      /* FM modulation depth */
    int32_t  fm_ratio;      /* FM carrier:modulator ratio */
} voice_t;

static voice_t voices[MAX_VOICES];

/* ---- Wavetable ROM pointer (memory-mapped QSPI flash) ---- */
#define WAVETABLE_BASE_ADDR  0x90000000UL
static const int16_t *wavetable_rom = (const int16_t *)WAVETABLE_BASE_ADDR;

/* ---- CV Input Values ---- */
static volatile uint16_t cv_values[4] = {0, 0, 0, 0};

/* ---- System Tick ---- */
static volatile uint32_t sys_tick_ms = 0;

/* ---- Function Prototypes ---- */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void I2C1_Init(void);
static void SPI1_Init(void);
static void I2S1_Init(void);
static void DMA_Init(void);
static void ADC1_Init(void);
static void UART4_Init(void);
static void VoiceAllocator_Init(void);
static void AudioEngine_Init(void);
static void AudioEngine_Process(int32_t *out_buf, int32_t *in_buf, uint32_t num_samples);
static void MIDI_ProcessByte(uint8_t byte);
static void SysTick_Handler(void);

/* ============================================
 * Main Entry Point
 * ============================================ */
int main(void) {
    /* 1. System clock: HSE 8 MHz → PLL → 480 MHz */
    SystemClock_Config();

    /* 2. Enable instruction and data caches */
    SCB_EnableICache();
    SCB_EnableDCache();

    /* 3. GPIO initialization */
    GPIO_Init();

    /* 4. Turn on power LED */
    LED_POWER_ON();
    LED_MIDI_OFF();

    /* 5. Peripheral initialization */
    I2C1_Init();
    SPI1_Init();

    /* 6. Initialize external devices via I2C */
    CODEC_RESET_LOW();
    for (volatile int i = 0; i < 100000; i++);  /* Wait 10 ms */
    CODEC_RESET_HIGH();
    for (volatile int i = 0; i < 100000; i++);  /* Wait 10 ms */

    /* 7. Configure codec */
    if (WM8778_Init() != 0) {
        /* Codec init failed — blink LED rapidly */
        while (1) {
            LED_POWER_ON();
            for (volatile int i = 0; i < 500000; i++);
            LED_POWER_OFF();
            for (volatile int i = 0; i < 500000; i++);
        }
    }

    /* 8. Configure headphone amp */
    TPA6130_Init();
    TPA6130_SetVolume(0x1F);  /* Moderate volume */

    /* 9. Initialize QSPI flash (for wavetable access) */
    SPI_Flash_Init();

    /* 10. Initialize voice allocator */
    VoiceAllocator_Init();

    /* 11. Initialize I2S and DMA for audio */
    I2S1_Init();
    DMA_Init();

    /* 12. Initialize ADC for CV inputs */
    ADC1_Init();

    /* 13. Initialize MIDI UART */
    UART4_Init();

    /* 14. Enable headphone amp */
    HPAMP_ENABLE();

    /* 15. Clear audio buffers */
    memset((void *)audio_tx_buf, 0, sizeof(audio_tx_buf));
    memset((void *)audio_rx_buf, 0, sizeof(audio_rx_buf));

    /* 16. Start SysTick (1 ms interval) */
    SysTick_Config(480000);  /* 480 MHz / 480000 = 1000 Hz */

    /* ======== Main Loop ======== */
    while (1) {
        /* Process MIDI input (polled from UART buffer) */
        if (UART4->ISR & USART_ISR_RXNE) {
            uint8_t midi_byte = (uint8_t)(UART4->RDR & 0xFF);
            MIDI_ProcessByte(midi_byte);
        }

        /* Sample CV inputs every 1 ms (triggered by ADC end-of-conversion) */
        if (ADC1->ISR & ADC_ISR_EOC) {
            cv_values[0] = ADC1->DR;  /* CV1 */
        }

        /* Background tasks */
        __WFI();  /* Wait for interrupt — low power */
    }
}

/* ============================================
 * System Clock Configuration
 * HSE 8 MHz → PLL1 → SYSCLK 480 MHz
 * ============================================ */
static void SystemClock_Config(void) {
    /* Enable HSE */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY))
        ;

    /* Configure PLL1: HSE/2 * 120 = 480 MHz */
    RCC->PLLCKSELR = (2 << 0)    /* PLL1M = 2 */
                    | (0 << 6);   /* Source = HSE */

    RCC->PLLCFGR = 0x0;           /* All fractional modes disabled */

    RCC->PLL1DIVR = (120 - 1) << 0   /* PLL1N = 120 */
                   | (1 - 1) << 9     /* PLL1P = 1 */
                   | (40 - 1) << 16   /* PLL1Q = 40 → 12 MHz for USB */
                   | (2 - 1) << 24;   /* PLL1R = 2 */

    /* Enable PLL1 */
    RCC->CR |= RCC_CR_PLL1ON;
    while (!(RCC->CR & RCC_CR_PLL1RDY))
        ;

    /* Configure flash latency for 480 MHz (7 WS) */
    FLASH->ACR = (7 << 0)      /* 7 wait states */
               | (1 << 8)      /* PREREAD enable */
               | (1 << 9);     /* ART enable */

    /* Switch SYSCLK to PLL1 */
    uint32_t cfgr = RCC->CFGR;
    cfgr &= ~(3 << 0);        /* SW = PLL1 */
    cfgr |= (0 << 0);         /* SW = PLL1 */
    cfgr &= ~(7 << 4);        /* AHB prescaler */
    cfgr |= (4 << 4);         /* AHB /2 = 240 MHz */
    cfgr &= ~(7 << 8);        /* APB1 prescaler */
    cfgr |= (4 << 8);         /* APB1 /2 = 120 MHz */
    cfgr &= ~(7 << 12);       /* APB2 prescaler */
    cfgr |= (4 << 12);        /* APB2 /2 = 120 MHz */
    cfgr &= ~(7 << 16);       /* APB4 prescaler */
    cfgr |= (4 << 16);         /* APB4 /2 = 120 MHz */
    RCC->CFGR = cfgr;

    /* Wait for clock switch */
    while ((RCC->CFGR & 0x3) != 0)
        ;

    /* Enable peripheral clocks */
    RCC->AHB1ENR |= (1 << 0);   /* DMA1 */
    RCC->AHB4ENR |= (1 << 0)    /* GPIOA */
                  | (1 << 1)    /* GPIOB */
                  | (1 << 2)    /* GPIOC */
                  | (1 << 4);   /* GPIOE */
    RCC->APB1LENR |= (1 << 6)   /* SPI1 (on APB2, see below) */
                   | (1 << 23); /* UART4 */
    RCC->APB2ENR |= (1 << 12)   /* SPI1 */
                  | (1 << 0);   /* TIM1 (optional) */
    RCC->AHB4ENR |= (1 << 5)    /* ADC1 */
                  | (1 << 16);  /* MDMA (optional) */
}

/* ============================================
 * I2C1 Initialization (400 kHz fast mode)
 * ============================================ */
static void I2C1_Init(void) {
    /* Enable I2C1 clock */
    RCC->APB1LENR |= (1 << 5);  /* I2C1EN */

    /* Wait for clock to stabilize */
    for (volatile int i = 0; i < 1000; i++);

    /* Reset I2C1 */
    RCC->APB1LRSTR |= (1 << 5);
    RCC->APB1LRSTR &= ~(1 << 5);

    /* Configure I2C1 timing for 400 kHz at 120 MHz APB1 */
    /* PRESC=3, SCLDEL=4, SDADEL=2, SCLH=17, SCLL=23 */
    I2C1->TIMINGR = (3 << 28)    /* PRESC = 3 */
                   | (4 << 20)    /* SCLDEL = 4 */
                   | (2 << 16)    /* SDADEL = 2 */
                   | (17 << 8)    /* SCLH = 17 */
                   | (23 << 0);   /* SCLL = 23 */

    I2C1->CR1 = (1 << 0);        /* PE = enable */
    I2C1->OAR1 = 0;               /* No own address (master only) */
}

/* ============================================
 * SPI1 Initialization (for W25Q256 flash)
 * ============================================ */
static void SPI1_Init(void) {
    /* Enable SPI1 clock */
    RCC->APB2ENR |= (1 << 12);  /* SPI1EN */

    /* Configure SPI1: master, 8-bit, mode 0, prescaler /8 → 15 MHz */
    SPI1->CR1 = (0 << 0)     /* CPHA = 0 */
              | (0 << 1)     /* CPOL = 0 */
              | (1 << 2)     /* MSTR = master */
              | (0 << 3)     /* BR[2:0] = /8 (120/8 = 15 MHz) */
              | (1 << 4)     /* BR[1] */
              | (0 << 5)     /* BR[0] — total prescaler /8 */
              | (0 << 7)     /* LSBFIRST = MSB */
              | (0 << 8)     /* SSM = software NSS */
              | (1 << 9)     /* SSI = 1 */
              | (0 << 10)    /* Full-duplex */
              | (0 << 11)    /* 8-bit data frame */
              | (1 << 14);   /* TXDMAEN (optional) */

    SPI1->CR2 = (0 << 0);       /* No CRC */

    /* Enable SPI1 */
    SPI1->CR1 |= (1 << 6);      /* SPE = enable */
}

/* ============================================
 * I2S1 Initialization (master transmit, 48 kHz, 24-bit, I2S format)
 * ============================================ */
static void I2S1_Init(void) {
    /* Enable I2S1 clock (via SPI1) */
    RCC->APB2ENR |= (1 << 12);  /* SPI1EN */

    /* Configure I2S1 as master transmit */
    /* I2SCFGR: I2S mode, 24-bit data, 32-bit channel, I2S standard */
    SPI_I2SCFGR = (1 << 0)      /* I2SMOD = I2S mode */
                | (2 << 1)      /* I2SCFG = master transmit */
                | (0 << 4)      /* I2SSTD = I2S Philips */
                | (0 << 5)      /* PCMSYNC = N/A */
                | (1 << 7)      /* CKPOL = high */
                | (6 << 8)      /* DATLEN = 24-bit */
                | (3 << 11);    /* CHLEN = 32-bit */

    /* I2S prescaler for 48 kHz MCLK = 256 * 48000 = 12.288 MHz */
    /* Using PLL2_Q clock source, prescaler calculated for exact rate */
    SPI_I2SPR = (5 << 0)       /* I2SDIV = 5 */
              | (1 << 8)       /* ODD = 1 → div = 2*(5) + 1 = 11 */
              | (1 << 9);      /* MCKOE = master clock output enable */

    /* Enable I2S */
    SPI_I2SCFGR |= (1 << 10);   /* I2SE = enable */
}

/* ============================================
 * DMA Initialization (for I2S1 TX/RX double-buffer)
 * ============================================ */
static void DMA_Init(void) {
    /* Enable DMA1 clock */
    RCC->AHB1ENR |= (1 << 0);   /* DMA1EN */

    /* Configure DMA1 Stream 5 for I2S1 TX */
    /* (Simplified — actual STM32H7 DMA configuration is extensive) */
    /* Circular mode, double-buffered, 32-bit word, memory-to-peripheral */
    DMA1_Stream5->CR = (0 << 6)    /* CHSEL = channel 0 */
                      | (1 << 10)   /* MSIZE = 32-bit */
                      | (1 << 12)   /* PSIZE = 32-bit */
                      | (1 << 14)   /* MINC = memory increment */
                      | (1 << 19)   /* CIRC = circular mode */
                      | (1 << 22)   /* DIR = memory-to-peripheral */
                      | (0 << 25);  /* PINC = no peripheral increment */

    DMA1_Stream5->PAR = (uint32_t)&(SPI1->DR);  /* Peripheral: SPI1 data register */
    DMA1_Stream5->M0AR = (uint32_t)audio_tx_buf;  /* Memory buffer 0 */
    DMA1_Stream5->NDTR = AUDIO_BUF_SAMPLES * AUDIO_BUF_CHANNELS * 2;
    DMA1_Stream5->FCR = 0x00000005;  /* FIFO threshold = full */

    /* Enable DMA stream */
    DMA1_Stream5->CR |= (1 << 0);  /* EN = enable */
}

/* ============================================
 * ADC1 Initialization (for CV inputs)
 * ============================================ */
static void ADC1_Init(void) {
    /* Enable ADC1 clock */
    RCC->AHB4ENR |= (1 << 5);   /* ADC12EN */

    /* Configure ADC1: 12-bit, single-ended, software trigger */
    ADC1->CFGR = (0 << 0)       /* AWD1 disabled */
               | (0 << 5)       /* Single conversion */
               | (0 << 13)      /* 12-bit resolution */
               | (1 << 14);     /* DMA circular mode (if needed) */

    /* Sample time: 24.5 cycles for all channels */
    ADC1->SMPR1 = (2 << 0)      /* SMP10 = 24.5 cycles */
                | (2 << 3)      /* SMP11 */
                | (2 << 6)      /* SMP12 */
                | (2 << 9);     /* SMP13 */

    /* Channel sequence: CH10, CH11, CH12, CH13 (CV1-CV4) */
    ADC1->SQR1 = (3 << 0)       /* 4 conversions */
               | (10 << 6)      /* 1st: CH10 (CV1 = PC0) */
               | (11 << 12)     /* 2nd: CH11 (CV2 = PC1) */
               | (12 << 18)     /* 3rd: CH12 (CV3 = PC2) */
               | (13 << 24);    /* 4th: CH13 (CV4 = PC13) */

    /* Enable ADC1 */
    ADC1->CR |= (1 << 0);       /* ADEN = enable */
    for (volatile int i = 0; i < 100; i++);  /* Wait for ADC ready */
}

/* ============================================
 * UART4 Initialization (MIDI 31250 baud)
 * ============================================ */
static void UART4_Init(void) {
    /* Enable UART4 clock */
    RCC->APB1LENR |= (1 << 23);  /* UART4EN */

    /* Configure UART4: 31250 baud, 8N1 */
    /* BRR = APB1_CLK / BAUD = 120000000 / 31250 = 3840 */
    UART4->BRR = 3840;

    UART4->CR1 = (1 << 0)      /* UE = enable */
               | (1 << 2)      /* RE = receiver enable */
               | (1 << 3)      /* TE = transmitter enable */
               | (0 << 12);    /* 8 data bits */

    UART4->CR2 = 0;             /* 1 stop bit */
    UART4->CR3 = 0;             /* No HW flow control */
}

/* ============================================
 * Voice Allocator Initialization
 * ============================================ */
static void VoiceAllocator_Init(void) {
    memset(voices, 0, sizeof(voices));
    for (int i = 0; i < MAX_VOICES; i++) {
        voices[i].active = 0;
        voices[i].note = 0xFF;
        voices[i].velocity = 0;
        voices[i].phase = 0;
        voices[i].phase_inc = 0;
        voices[i].wavetable_idx = 0;
        voices[i].osc_mode = OSC_MODE_WAVETABLE;
        voices[i].fm_depth = 0;
        voices[i].fm_ratio = 1;
    }
}

/* ============================================
 * Audio Engine Process (called from DMA half/full interrupt)
 * Generates `num_samples` stereo frames into `out_buf`
 * ============================================ */
static void AudioEngine_Process(int32_t *out_buf, int32_t *in_buf,
                                 uint32_t num_samples) {
    (void)in_buf;  /* Unused for now (line-in passthrough not implemented) */

    for (uint32_t i = 0; i < num_samples; i++) {
        int32_t mix_l = 0;
        int32_t mix_r = 0;

        /* Mix all active voices */
        for (int v = 0; v < MAX_VOICES; v++) {
            if (!voices[v].active) continue;

            /* Advance phase accumulator */
            voices[v].phase += voices[v].phase_inc;

            /* Calculate sample from wavetable */
            uint32_t phase_idx = voices[v].phase >> 16;  /* 16-bit fractional */
            int16_t sample = wavetable_rom[voices[v].wavetable_idx * MAX_WAVETABLE_SIZE
                                           + (phase_idx % MAX_WAVETABLE_SIZE)];

            /* Apply velocity scaling (8-bit velocity → gain) */
            int32_t gain = voices[v].velocity << 8;  /* Scale to 16-bit range */
            int32_t scaled = ((int32_t)sample * gain) >> 8;

            /* Stereo mix (simple: center panning) */
            mix_l += scaled;
            mix_r += scaled;
        }

        /* Clamp to 24-bit range */
        if (mix_l > 0x7FFFFF) mix_l = 0x7FFFFF;
        if (mix_l < -0x800000) mix_l = -0x800000;
        if (mix_r > 0x7FFFFF) mix_r = 0x7FFFFF;
        if (mix_r < -0x800000) mix_r = -0x800000;

        /* Pack into I2S format (32-bit, 24-bit left-aligned) */
        out_buf[i * 2]     = mix_l << 8;  /* Left channel */
        out_buf[i * 2 + 1] = mix_r << 8;  /* Right channel */
    }
}

/* ============================================
 * MIDI Note-On Frequency Calculator
 * Converts MIDI note number to phase increment
 * ============================================ */
static uint32_t MIDI_NoteToPhaseInc(uint8_t note) {
    /* A4 = MIDI 69 = 440 Hz
     * phase_inc = (freq * 2^32) / sample_rate
     * freq = 440 * 2^((note-69)/12) */
    float freq = 440.0f;
    float semitones = (float)(note - 69);
    float multiplier = 1.0f;
    int whole = (int)semitones;
    float frac = semitones - whole;

    /* Integer power of 2^(whole/12) using lookup */
    static const float pow2_table[25] = {
        1.000000f, 1.059463f, 1.122462f, 1.189207f, 1.259921f,
        1.334840f, 1.414214f, 1.498307f, 1.587401f, 1.681793f,
        1.781797f, 1.887749f, 2.000000f, 2.118926f, 2.244924f,
        2.378414f, 2.519842f, 2.669680f, 2.828427f, 2.996615f,
        3.174802f, 3.363586f, 3.563595f, 3.775498f, 4.000000f
    };

    /* Handle negative notes (frequencies below A4) */
    if (whole >= 0) {
        multiplier = pow2_table[whole % 12] * (1 << (whole / 12));
    } else {
        whole = -whole;
        multiplier = 1.0f / (pow2_table[whole % 12] * (1 << (whole / 12)));
    }

    /* Interpolate fractional part */
    int idx = (int)(frac * 12.0f);
    if (idx < 0) idx = 0;
    if (idx > 11) idx = 11;
    float frac_mult = pow2_table[idx];

    freq = 440.0f * multiplier * frac_mult;

    /* Convert to phase increment: phase_inc = freq * (2^32 / 48000) */
    uint32_t phase_inc = (uint32_t)(freq * (4294967296.0f / 48000.0f));
    return phase_inc;
}

/* ============================================
 * MIDI Byte Parser (state machine)
 * ============================================ */
static uint8_t midi_status = 0;
static uint8_t midi_data[2] = {0, 0};
static uint8_t midi_data_idx = 0;
static uint8_t midi_running_status = 0;

static void MIDI_ProcessByte(uint8_t byte) {
    LED_MIDI_ON();

    if (byte & 0x80) {
        /* Status byte */
        midi_status = byte;
        midi_running_status = byte;
        midi_data_idx = 0;
    } else {
        /* Data byte */
        midi_status = midi_running_status;
        midi_data[midi_data_idx++] = byte;

        uint8_t msg_type = midi_status & 0xF0;
        uint8_t channel = midi_status & 0x0F;

        switch (msg_type) {
        case 0x80: /* Note Off */
            if (midi_data_idx >= 2) {
                uint8_t note = midi_data[0];
                /* Find and deactivate voice */
                for (int v = 0; v < MAX_VOICES; v++) {
                    if (voices[v].active && voices[v].note == note
                        && voices[v].channel == channel) {
                        voices[v].active = 0;
                        voices[v].velocity = 0;
                        break;
                    }
                }
                midi_data_idx = 0;
            }
            break;

        case 0x90: /* Note On */
            if (midi_data_idx >= 2) {
                uint8_t note = midi_data[0];
                uint8_t velocity = midi_data[1];

                if (velocity == 0) {
                    /* Note On with velocity 0 = Note Off */
                    for (int v = 0; v < MAX_VOICES; v++) {
                        if (voices[v].active && voices[v].note == note
                            && voices[v].channel == channel) {
                            voices[v].active = 0;
                        }
                    }
                } else {
                    /* Allocate voice */
                    int free_voice = -1;
                    for (int v = 0; v < MAX_VOICES; v++) {
                        if (!voices[v].active) {
                            free_voice = v;
                            break;
                        }
                    }
                    if (free_voice >= 0) {
                        voices[free_voice].active = 1;
                        voices[free_voice].note = note;
                        voices[free_voice].velocity = velocity >> 1;  /* Scale to 0-127 → 0-63 */
                        voices[free_voice].channel = channel;
                        voices[free_voice].phase = 0;
                        voices[free_voice].phase_inc = MIDI_NoteToPhaseInc(note);
                        voices[free_voice].wavetable_idx = 0;  /* Default: first wavetable */
                    }
                }
                midi_data_idx = 0;
            }
            break;

        case 0xB0: /* Control Change */
            if (midi_data_idx >= 2) {
                /* CC processing (future: filter cutoff, envelope, etc.) */
                midi_data_idx = 0;
            }
            break;

        case 0xC0: /* Program Change */
            if (midi_data_idx >= 1) {
                /* Change wavetable/patch */
                midi_data_idx = 0;
            }
            break;

        case 0xE0: /* Pitch Bend */
            if (midi_data_idx >= 2) {
                /* Pitch bend processing */
                midi_data_idx = 0;
            }
            break;

        default:
            midi_data_idx = 0;
            break;
        }
    }

    LED_MIDI_OFF();
}

/* ============================================
 * SysTick Handler (1 ms interval)
 * ============================================ */
void SysTick_Handler(void) {
    sys_tick_ms++;

    /* Trigger ADC scan every 1 ms */
    ADC1->CR |= (1 << 2);  /* ADSTART = start conversion */
}