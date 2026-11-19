# -*- coding: utf-8 -*-
import re
import sys

palette_path = 'G:/TBuild/tdesktop/Telegram/lib_ui/ui/colors.palette'
all_keys_path = 'G:/TBuild/tdesktop/Telegram/SourceFiles/ui/all_keys.txt'

palette_entries = []
with open(palette_path, 'r', encoding='utf-8') as f:
    for line in f:
        line = line.strip()
        if not line or line.startswith('//'): continue
        m = re.match(r'^([a-zA-Z0-9_]+)\s*:\s*([^;]+);(?:\s*//\s*(.*))?$', line)
        if m:
            palette_entries.append((m.group(1), m.group(2).strip(), (m.group(3) or '').strip()))

raw = {k: v for k, v, c in palette_entries}
comments = {k: c for k, v, c in palette_entries}

def resolve_alpha(key, visited=None):
    if visited is None: visited = set()
    if key in visited: return 255
    visited.add(key)
    val = raw.get(key, '')
    if '|' in val: val = val.split('|')[0].strip()
    if val.startswith('#'):
        h = val[1:]
        if len(h) == 8: return int(h[6:8], 16)
        elif len(h) == 4: return int(h[3]*2, 16)
        else: return 255
    elif val in raw:
        return resolve_alpha(val, visited)
    return 255

alphas = {k: resolve_alpha(k) for k in raw}
print(f"Loaded {len(palette_entries)} keys from palette. Alphas resolved.")
