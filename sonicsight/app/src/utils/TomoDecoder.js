/**
 * TomoDecoder.js — SonicSight Companion
 * Decodes compressed tomogram data from device binary format
 * into a float array for rendering.
 * @author jayis1
 */

/**
 * Decode a chunk of tomogram data from the device's hex-encoded format.
 *
 * Device format per notification:
 *   "AT+NOTIFY=2,<chunk_offset>:<hex_encoded_pixels>\\r\\n"
 * where each pixel is one hex byte (00–FF) representing velocity index.
 *
 * @param {string} text - Raw notification text from device.
 * @param {number} gridSize - Reconstruction grid dimension (default 64).
 * @returns {{ image: Float32Array, offset: number, gridSize: number } | null}
 */
export function decodeTomogramChunk(text, gridSize = 64) {
  if (!text || typeof text !== 'string') return null;

  /* Strip AT+NOTIFY prefix */
  const dataStart = text.indexOf(',');
  if (dataStart < 0) return null;

  const dataPart = text.substring(dataStart + 1).trim();

  /* Split on ':' to get offset and hex data */
  const colonIdx = dataPart.indexOf(':');
  if (colonIdx < 0) return null;

  const offset = parseInt(dataPart.substring(0, colonIdx), 10);
  const hexData = dataPart.substring(colonIdx + 1);

  if (isNaN(offset) || !hexData) return null;

  const pixels = [];
  for (let i = 0; i < hexData.length; i += 2) {
    const byte = parseInt(hexData.substr(i, 2), 16);
    if (!isNaN(byte)) {
      /* Convert 8-bit index back to velocity (m/s) */
      const velocity = 500 + byte * 13.73;
      pixels.push(velocity);
    }
  }

  return { pixels, offset, gridSize };
}

/**
 * Assemble multiple chunks into a complete tomogram image.
 * @param {Array<{pixels: number[], offset: number}>} chunks
 * @param {number} gridSize
 * @returns {Float32Array}
 */
export function assembleTomogram(chunks, gridSize = 64) {
  const image = new Float32Array(gridSize * gridSize);

  for (const chunk of chunks) {
    for (let i = 0; i < chunk.pixels.length; i++) {
      const idx = chunk.offset + i;
      if (idx < image.length) {
        image[idx] = chunk.pixels[i];
      }
    }
  }

  return image;
}

/**
 * Compute statistics on a tomogram image.
 * @param {Float32Array} image
 * @param {number} gridSize
 * @returns {{ min: number, max: number, mean: number, std: number }}
 */
export function computeTomogramStats(image, gridSize = 64) {
  let min = Infinity, max = -Infinity;
  let sum = 0, sumSq = 0, count = 0;

  for (let i = 0; i < image.length; i++) {
    const v = image[i];
    if (v > 0) {
      if (v < min) min = v;
      if (v > max) max = v;
      sum += v;
      sumSq += v * v;
      count++;
    }
  }

  const mean = count > 0 ? sum / count : 0;
  const variance = count > 0 ? (sumSq / count) - (mean * mean) : 0;

  return {
    min: min === Infinity ? 0 : min,
    max: max === -Infinity ? 0 : max,
    mean,
    std: Math.sqrt(Math.max(0, variance)),
  };
}

export default {
  decodeTomogramChunk,
  assembleTomogram,
  computeTomogramStats,
};