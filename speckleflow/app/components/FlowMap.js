/**
 * FlowMap.js — Canvas-based flow-map renderer component
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Renders an 8-bit flow map (640×480) to a React Native view using
 * a WebView canvas. The flow values are colorized using the selected
 * colormap LUT and drawn as an ImageData object.
 */

import React, { useRef, useEffect, memo } from 'react';
import { View, StyleSheet, Dimensions } from 'react-native';
import { WebView } from 'react-native-webview';
import { COLORMAPS } from '../utils/protocol';

const SCREEN_WIDTH = Dimensions.get('window').width;

function FlowMap({ flowMap, colormap, roi, width = 640, height = 480 }) {
  const webViewRef = useRef(null);

  useEffect(() => {
    if (!flowMap || !webViewRef.current) return;

    const lut = COLORMAPS[colormap] || COLORMAPS.jet;
    // Build RGB pixel data
    const pixels = new Uint8ClampedArray(width * height * 4);
    for (let i = 0; i < width * height; i++) {
      const k = flowMap[i] || 0;
      pixels[i * 4]     = lut[k * 3];      // R
      pixels[i * 4 + 1] = lut[k * 3 + 1];  // G
      pixels[i * 4 + 2] = lut[k * 3 + 2];  // B
      pixels[i * 4 + 3] = 255;             // A
    }

    // Draw ROI overlay
    if (roi) {
      const { x, y, w, h } = roi;
      // Draw rectangle border (green)
      const drawRect = (px, py, pw, ph, r, g, b) => {
        for (let i = px; i < px + pw; i++) {
          for (let dy = 0; dy < 2; dy++) {
            setPixel(pixels, i, py + dy, width, height, r, g, b);
            setPixel(pixels, i, py + ph - 1 - dy, width, height, r, g, b);
          }
        }
        for (let j = py; j < py + ph; j++) {
          for (let dx = 0; dx < 2; dx++) {
            setPixel(pixels, px + dx, j, width, height, r, g, b);
            setPixel(pixels, px + pw - 1 - dx, j, width, height, r, g, b);
          }
        }
      };
      drawRect(x, y, w, h, 0, 255, 0);
    }

    // Send pixel data to WebView via postMessage
    // We base64-encode the pixel data for transfer
    const base64 = base64Encode(pixels);
    webViewRef.current.postMessage(JSON.stringify({
      type: 'render',
      width,
      height,
      data: base64,
    }));
  }, [flowMap, colormap, roi, width, height]);

  const html = `
    <!DOCTYPE html>
    <html>
    <head>
      <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1">
      <style>
        body { margin: 0; padding: 0; background: #000; }
        canvas { width: 100%; height: 100%; image-rendering: pixelated; }
      </style>
    </head>
    <body>
      <canvas id="canvas"></canvas>
      <script>
        const canvas = document.getElementById('canvas');
        const ctx = canvas.getContext('2d');
        canvas.width = ${width};
        canvas.height = ${height};

        window.addEventListener('message', function(e) {
          const msg = JSON.parse(e.data);
          if (msg.type === 'render') {
            const binary = atob(msg.data);
            const pixels = new Uint8ClampedArray(binary.length);
            for (let i = 0; i < binary.length; i++) {
              pixels[i] = binary.charCodeAt(i);
            }
            const imageData = new ImageData(pixels, msg.width, msg.height);
            ctx.putImageData(imageData, 0, 0);
          }
        });

        // Signal ready
        window.ReactNativeWebView && window.ReactNativeWebView.postMessage('ready');
      </script>
    </body>
    </html>
  `;

  return (
    <View style={styles.container}>
      <WebView
        ref={webViewRef}
        source={{ html }}
        style={styles.webview}
        javaScriptEnabled
        scrollEnabled={false}
        bounces={false}
      />
    </View>
  );
}

function setPixel(pixels, x, y, w, h, r, g, b) {
  if (x < 0 || x >= w || y < 0 || y >= h) return;
  const idx = (y * w + x) * 4;
  pixels[idx] = r;
  pixels[idx + 1] = g;
  pixels[idx + 2] = b;
  pixels[idx + 3] = 255;
}

// Simple base64 encoder for Uint8ClampedArray
function base64Encode(bytes) {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  let result = '';
  const len = bytes.length;
  for (let i = 0; i < len; i += 3) {
    const b0 = bytes[i];
    const b1 = i + 1 < len ? bytes[i + 1] : 0;
    const b2 = i + 2 < len ? bytes[i + 2] : 0;
    result += chars[b0 >> 2];
    result += chars[((b0 & 3) << 4) | (b1 >> 4)];
    result += i + 1 < len ? chars[((b1 & 15) << 2) | (b2 >> 6)] : '=';
    result += i + 2 < len ? chars[b2 & 63] : '=';
  }
  return result;
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#000',
    borderRadius: 8,
    overflow: 'hidden',
  },
  webview: {
    flex: 1,
    backgroundColor: '#000',
  },
});

export default memo(FlowMap);