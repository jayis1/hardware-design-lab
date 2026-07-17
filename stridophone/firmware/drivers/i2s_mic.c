/*
 * i2s_mic.c — 4-channel TDM MEMS microphone driver using SAI1 + BDMA.
 *
 * Author : jayis1
 * License: MIT
 *
 * The four Knowles SPH0641LU4H-1 mics share a single I2S/TDM bus in a 4-slot
 * configuration. SAI1 block A is configured as master clock + bit/frame
 * clock generator; block B is the data receiver. The BDMA peripheral pumps
 * the SAI RX FIFO into a double-buffered ring in internal SRAM3 (tightly
 * coupled, low latency).
 *
 * Slot layout (96 bits per slot at 24-bit data, 48 kHz FS):
 *   Slot0 = Mic0, Slot1 = Mic1, Slot2 = Mic2, Slot3 = Mic3
 * Frame sync is 256-bit wide (4 slots * 64 bits padded).
 */
#include "i2s_mic.h"
#include "../registers.h"
#include <string.h>

/* ---- Internal DMA ring ------------------------------------------- *
 * Two half-buffers, each holding AUDIO_BLOCK samples * 4 channels of
 * 32-bit words (24-bit data left-justified in a 32-bit slot). The BDMA
 * runs in circular mode; half-transfer and complete IRQs flag the app. */
#define RING_WORDS (AUDIO_BLOCK * AUDIO_NCH * 2)
static int32_t g_ring[RING_WORDS] __attribute__((aligned(32)));
static volatile int g_ring_pos = 0;   /* index of next word to read */

/* MCLK divider for 12.288 MHz from a 1.024 MHz SAI kernel clock (PLL3 out
 * routed to SAI). We pick NODIV=0 and MCKDIV so that MCLK = FS * 256.
 * For FS=48k, MCLK = 12.288 MHz. With SAI kernel = 12.288 MHz and MCKDIV=0
 * the audio block runs at the kernel directly. */
#define SAI_MCKDIV_VAL  0

void i2s_mic_init(void) {
    /* Enable SAI1 clock. */
    RCC->APB2ENR |= RCC_APB2ENR_SAI1;

    /* ---- SAI1 Block A: master (generates FS, SCK, MCLK) ----------- */
    SAI1->A_CR1 = (0U<<0)        /* SAIEN off while configuring */
                | (1U<<2)        /* MODE[1:0] = master RX */
                | (0U<<5)        /* PRTCFG = free */
                | (3U<<20)       /* MCKDIV (set later) */
                | (1U<<17);      /* DMAEN */
    /* Sync: no sync, block A is self-master. */
    SAI1->A_CR2 = (0U<<0)        /* FTH: FIFO threshold = 1/4 */
                | (2U<<2)        /* FLRL: frame length 256 bits (4*64) */
                | (1U<<7);       /* FSOFF: FS active, edge=configurable */
    /* Frame: 256-bit, 4 slots of 64 bits, FS pulse at start. */
    SAI1->A_FRCR = (255U<<0)     /* FRL-1 = 255 */
                | (0U<<8)        /* FSALL-1 = 0 (FS is a 1-bit pulse? set below) */
                | (1U<<16)       /* FSDEF: FS active high during slot 0 */
                | (0U<<17)       /* FSPOL: FS active high */
                | (1U<<18);      /* FSOFF: FS asserted on first bit */
    /* Slots A: enable all 4, each 32-bit wide. */
    SAI1->A_SLOTR = (0xFU<<0)    /* slots 0-3 enabled */
                  | (32U<<6)     /* slot size 32 bits (SLOTSZ) */
                  | (3U<<16);    /* NBSLOT-1 = 3 (4 slots) */
    SAI1->A_IM   = 0;            /* we use DMA, not SAI IRQs */
    SAI1->A_CLRFR = 0xFFFFFFFFU;

    /* ---- SAI1 Block B: slave receiver for the TDM data line ------- *
     * We actually receive on block B's SD input which is wired to the
     * TDM data from the mic array. Configure B as slave to A's clocks. */
    SAI1->B_CR1 = (1U<<2)        /* MODE = slave RX */
                | (1U<<17);      /* DMAEN */
    SAI1->B_CR2 = (0U<<0) | (2U<<2);
    SAI1->B_FRCR = SAI1->A_FRCR;
    SAI1->B_SLOTR = SAI1->A_SLOTR;
    SAI1->B_IM = 0;
    SAI1->B_CLRFR = 0xFFFFFFFFU;

    /* ---- BDMA channel 0: SAI1 RX -> SRAM ring, circular, dual event */
    /* Enable BDMA clock. */
    RCC->AHB4ENR |= (1U<<24);  /* BDMAEN (bit in AHB4ENR on H7). */

    BDMA->C0PAR  = (uint32_t)&SAI1->B_DR;     /* peripheral: SAI1 block B DR */
    BDMA->C0MAR  = (uint32_t)g_ring;           /* memory: ring buffer */
    BDMA->C0NDTR = RING_WORDS;                 /* total words */
    BDMA->C0CR   = BDMA_CR_P2M
                 | BDMA_CR_CIRC
                 | BDMA_CR_MINC
                 | (2U<<9)     /* PSIZE 32-bit */
                 | (2U<<11)    /* MSIZE 32-bit */
                 | BDMA_CR_TCIE
                 | BDMA_CR_HTIE;

    /* Enable BDMA channel 0 IRQ. */
    nvic_enable(BDMA_CH0_IRQ);

    /* Reset ring pointer. */
    g_ring_pos = 0;
}

void i2s_mic_start(void) {
    /* Enable SAI blocks: B (slave) first, then A (master). */
    SAI1->B_CR1 |= SAI_CR1_SAIEN;
    SAI1->A_CR1 |= SAI_CR1_SAIEN;
    /* Kick the BDMA. */
    BDMA->C0CR |= BDMA_CR_EN;
}

void i2s_mic_stop(void) {
    BDMA->C0CR &= ~BDMA_CR_EN;
    SAI1->A_CR1 &= ~SAI_CR1_SAIEN;
    SAI1->B_CR1 &= ~SAI_CR1_SAIEN;
    gpio_clr(MIC_EN_PORT, MIC_EN_PAD);
}

/* Deinterleave the most recent N samples per channel. The ring is laid out
 * as [slot0_word, slot1_word, slot2_word, slot3_word, slot0_word, ...]
 * repeating, so we stride by 4 and read the last N words per channel. */
void i2s_mic_deinterleave(float dst[4][DSP_FRAME_N], int n) {
    /* Snapshot the current NDTR to know where BDMA is writing. */
    int remaining = (int)BDMA->C0NDTR;
    int write_pos = RING_WORDS - remaining;  /* index BDMA will write next */

    /* We want the N samples *before* write_pos (the freshest complete
     * block), i.e. start = write_pos - N*4. */
    int start = write_pos - n * AUDIO_NCH;
    if (start < 0) start += RING_WORDS;

    for (int i = 0; i < n; i++) {
        for (int ch = 0; ch < AUDIO_NCH; ch++) {
            int idx = start + i * AUDIO_NCH + ch;
            if (idx >= RING_WORDS) idx -= RING_WORDS;
            int32_t raw = g_ring[idx];
            /* 24-bit data left-justified in 32-bit slot; shift down. */
            int32_t q = raw >> 8;
            dst[ch][i] = (float)q / (float)(1 << 23);  /* normalise to [-1,1] */
        }
    }
}

const int32_t *i2s_mic_ring_raw(void) { return g_ring; }
int            i2s_mic_ring_capacity(void) { return RING_WORDS; }