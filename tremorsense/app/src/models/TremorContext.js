/*
 * TremorContext.js — React Context for app-wide state
 * TremorSense — Wearable Tremor Characterization Band
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React, { createContext, useState } from 'react';

export const TremorContext = createContext({
  bleState: 'disconnected',
  deviceData: null,
  theme: null,
});

export function TremorProvider({ value, children }) {
  return (
    <TremorContext.Provider value={value}>
      {children}
    </TremorContext.Provider>
  );
}