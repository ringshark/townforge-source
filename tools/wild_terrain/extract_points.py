# Extracts every wilderness gameplay position from main.cpp into wildpts.json.
import re, json
s = open('../../main.cpp').read()
def table(name):
    m = re.search(name + r'\s*=\s*\{\{(.*?)\n\}\};', s, re.S)
    return m.group(1) if m else ''
out = {}
for name in ['kWildernessGatherNodes', 'kWildernessCreatureSpots', 'kWildernessMonsterSpots',
             'kWildernessDungeonEntrances', 'kWildernessFoliage', 'kWildernessInnocentSpots',
             'kHousePlots', 'kShrines', 'kRivalCampSpots', 'kSaltDocks', 'kKingsRoadWaypoints']:
    out[name] = [(int(a), int(b)) for a, b in re.findall(r'\{\s*\{?\s*(\d+)\s*,\s*(\d+)\s*\}', table(name))]
json.dump(out, open('wildpts.json', 'w'))
print({k: len(v) for k, v in out.items()})
