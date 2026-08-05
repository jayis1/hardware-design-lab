// babel.config.js — FrostSentinel app Babel config
// Author: jayis1 — Copyright (C) 2026 jayis1 — MIT License
module.exports = function(api) {
  api.cache(true);
  return {
    presets: ['babel-preset-expo'],
  };
};