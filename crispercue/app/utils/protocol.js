// CrisperCue protocol utilities
// Author: jayis1
export const mockBins = [
  {
    id: 'drawer-a',
    name: 'Ripening Fruit Bin',
    produce: 'peaches + avocados',
    freshness: 61,
    spoilageRisk: 0.49,
    ethylene: 0.66,
    co2: 1410,
    massG: 488,
    recommendation: 'Move avocados to counter and use peaches within 36 hours.',
    recipeIdeas: ['stone-fruit salsa', 'breakfast compote', 'sheet-pan roast fruit'],
  },
  {
    id: 'drawer-b',
    name: 'Leafy Greens Bin',
    produce: 'romaine + herbs',
    freshness: 84,
    spoilageRisk: 0.16,
    ethylene: 0.08,
    co2: 940,
    massG: 372,
    recommendation: 'Humidity is ideal; keep purge fan low to preserve crispness.',
    recipeIdeas: ['green goddess wraps', 'herb salad', 'soup garnish packs'],
  },
  {
    id: 'drawer-c',
    name: 'Berry Rescue Bin',
    produce: 'strawberries + blueberries',
    freshness: 47,
    spoilageRisk: 0.63,
    ethylene: 0.11,
    co2: 1190,
    massG: 218,
    recommendation: 'Sort bruised fruit now and queue a jam or smoothie batch.',
    recipeIdeas: ['quick jam', 'baked oats topping', 'freezer smoothie packs'],
  },
];

export function getBinHealth(bin) {
  if (bin.freshness >= 80) {
    return { label: 'Stable', accent: '#4EE5A0' };
  }
  if (bin.freshness >= 55) {
    return { label: 'Watch', accent: '#FFD166' };
  }
  return { label: 'Rescue', accent: '#FF6B6B' };
}

export function buildCommandPacket(bin, options) {
  return JSON.stringify({
    author: 'jayis1',
    target: bin.id,
    fanBoost: options.fanBoost,
    notifications: options.notifications,
    suggestedAction: bin.recommendation,
  });
}
