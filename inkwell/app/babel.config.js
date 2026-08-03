// babel.config.js — Babel preset config for the Inkwell companion app
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

module.exports = function(api) {
  api.cache(true);
  return {
    presets: ['babel-preset-expo'],
  };
};