/**
 * cgh_pipeline.h — CGH Compute Pipeline Driver Header
 *
 * Driver for the Computer-Generated Holography compute pipeline
 * in the PhotonWeave FPGA. Manages parameter configuration,
 * frame submission, and hologram retrieval.
 */

#ifndef CGH_PIPELINE_H
#define CGH_PIPELINE_H

#include <stdint.h>
#include <stdbool.h>

//=============================================================================
// CGH Parameter Set Structure
//=============================================================================
typedef struct {
    uint32_t wavelength_nm_x1000;    // e.g., 532000 for 532nm green
    uint32_t depth_min_um;          // Minimum depth in micrometers
    uint32_t depth_max_um;          // Maximum depth in micrometers
    uint32_t depth_planes;          // Number of depth planes (1-256)
    uint32_t input_width;           // Depth map width in pixels
    uint32_t input_height;          // Depth map height in pixels
    uint32_t output_width;          // Hologram width in pixels
    uint32_t output_height;         // Hologram height in pixels
    uint32_t fft_size;              // FFT size (must be power of 2)
    uint32_t propagation_model;     // 0=Fresnel, 1=Angular Spectrum, 2=Fraunhofer
    uint32_t phase_quantize_bits;   // Output quantization: 8 or 10 bits
    uint32_t color_channel;         // 0=Red(638nm), 1=Green(532nm), 2=Blue(450nm)
    uint32_t pixel_pitch_um_x100;   // SLM pixel pitch in µm × 100
    uint32_t fill_factor_percent;   // SLM fill factor (0-100)
} cgh_params_t;

//=============================================================================
// CGH Pipeline Status Structure
//=============================================================================
typedef struct {
    bool     idle;              // Pipeline idle, ready for new frame
    bool     busy;              // Pipeline actively processing
    bool     frame_done;        // Frame complete, hologram ready
    bool     error;             // Error condition active
    uint32_t error_flags;       // Detailed error flags
    uint32_t frame_count;       // Total frames processed since reset
    uint32_t frame_time_us;     // Last frame processing time in µs
    uint32_t fps_x1000;         // Current frames per second × 1000
    uint8_t  active_engines;    // Number of FFT engines active
} cgh_status_t;

//=============================================================================
// Propagation Models
//=============================================================================
#define CGH_PROP_FRESNEL            0
#define CGH_PROP_ANGULAR_SPECTRUM   1
#define CGH_PROP_FRAUNHOFER         2

//=============================================================================
// Color Channels
//=============================================================================
#define CGH_COLOR_RED               0   // 638 nm reference
#define CGH_COLOR_GREEN             1   // 532 nm reference
#define CGH_COLOR_BLUE              2   // 450 nm reference

//=============================================================================
// Default Wavelengths (nm × 1000)
//=============================================================================
#define CGH_WAVELENGTH_RED          638000
#define CGH_WAVELENGTH_GREEN        532000
#define CGH_WAVELENGTH_BLUE         450000

//=============================================================================
// CGH Error Flags
//=============================================================================
#define CGH_ERR_DDR4_READ           0x0001
#define CGH_ERR_DDR4_WRITE          0x0002
#define CGH_ERR_FFT_OVERFLOW        0x0004
#define CGH_ERR_FFT_TIMEOUT         0x0008
#define CGH_ERR_PARAM_INVALID       0x0010
#define CGH_ERR_DMA_FAULT           0x0020
#define CGH_ERR_BUFFER_OVERRUN      0x0040
#define CGH_ERR_WATCHDOG            0x0080
#define CGH_ERR_FFT_ENGINE_MASK     0xFF00  // Per-engine fault bits

//=============================================================================
// CGH Control Register Bits
//=============================================================================
#define CGH_CTRL_START              0x01
#define CGH_CTRL_ABORT              0x02
#define CGH_CTRL_PARAM_SET_SHIFT    16
#define CGH_CTRL_PARAM_SET_MASK     (0x0F << 16)
#define CGH_CTRL_FFT_ENGINE_SHIFT   20
#define CGH_CTRL_FFT_ENGINE_MASK    (0xFF << 20)

//=============================================================================
// CGH Status Register Bits
//=============================================================================
#define CGH_STATUS_IDLE             0x01
#define CGH_STATUS_BUSY             0x02
#define CGH_STATUS_FRAME_DONE       0x04
#define CGH_STATUS_ERROR            0x08

//=============================================================================
// Function Prototypes
//=============================================================================

/**
 * Initialize the CGH pipeline.
 * Verifies device presence, checks FPGA/DDR4 readiness,
 * resets pipeline, and configures default parameters.
 * @return 0 on success, negative error code on failure
 */
int cgh_pipeline_init(void);

/**
 * Configure a CGH parameter set.
 * Up to 16 parameter sets can be stored (0-15).
 * @param params Pointer to parameter structure
 * @param param_set Parameter set index (0-15)
 * @return 0 on success, negative on validation error
 */
int cgh_pipeline_configure(const cgh_params_t *params, uint8_t param_set);

/**
 * Start CGH pipeline processing on the current frame.
 * Pipeline must be idle before calling.
 * @return 0 on success, -1 if already busy
 */
int cgh_pipeline_start(void);

/**
 * Abort the current frame processing.
 * Pipeline returns to idle state.
 * @return 0 on success, -1 on timeout
 */
int cgh_pipeline_abort(void);

/**
 * Get current pipeline status.
 * @param status Pointer to status structure to fill
 */
void cgh_pipeline_get_status(cgh_status_t *status);

/**
 * Wait for frame processing to complete.
 * @param timeout_ms Maximum wait time in milliseconds
 * @return 0 on frame done, -1 on error, -2 on timeout
 */
int cgh_pipeline_wait_frame_done(uint32_t timeout_ms);

/**
 * Submit a depth map for hologram computation.
 * @param ddr4_addr 64-bit DDR4 address of depth map data
 * @param width Depth map width in pixels
 * @param height Depth map height in pixels
 * @param color_channel Color channel (0=R, 1=G, 2=B)
 * @return 0 on success, negative on error
 */
int cgh_pipeline_submit_depth_map(uint64_t ddr4_addr, uint32_t width,
                                   uint32_t height, uint32_t color_channel);

/**
 * Retrieve the computed hologram buffer address.
 * @param ddr4_addr Pointer to receive DDR4 address of hologram
 * @return 0 on success, -1 on parameter error, -2 if no frame ready
 */
int cgh_pipeline_retrieve_hologram(uint64_t *ddr4_addr);

/**
 * Set which FFT engines are active.
 * @param mask Bitmask of active engines (0xFF = all 8)
 */
void cgh_pipeline_set_fft_engine_mask(uint8_t mask);

/**
 * Soft reset the CGH pipeline.
 * @return 0 if pipeline returned to idle, -1 on failure
 */
int cgh_pipeline_soft_reset(void);

/**
 * Get the recommended FFT size for given output dimensions.
 * @param output_width Desired output width
 * @param output_height Desired output height
 * @return Recommended FFT size (power of 2)
 */
uint32_t cgh_pipeline_recommended_fft_size(uint32_t output_width,
                                            uint32_t output_height);

/**
 * Validate CGH parameters for correctness.
 * @param params Pointer to parameter structure
 * @return 0 if valid, negative error code if invalid
 */
int cgh_pipeline_validate_params(const cgh_params_t *params);

#endif // CGH_PIPELINE_H
