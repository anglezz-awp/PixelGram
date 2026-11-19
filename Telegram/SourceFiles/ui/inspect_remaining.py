# -*- coding: utf-8 -*-
import sys
sys.path.append("G:/TBuild/tdesktop/Telegram/SourceFiles/ui")
import test_gen

with open("G:/TBuild/tdesktop/Telegram/SourceFiles/ui/all_keys.txt", "r", encoding="utf-8") as f:
    all_keys = [line.strip().split("\t")[0] for line in f if line.strip()]

unmapped = [k for k in all_keys if k not in test_gen.EXPLICIT_MAP]
from collections import defaultdict
grouped = defaultdict(list)
for k in unmapped:
    import re
    m = re.match(r"^([a-z]+|[A-Z][a-z]+)", k)
    p = m.group(1) if m else "other"
    grouped[p].append(k)

print(f"Remaining unmapped: {len(unmapped)}")
for p, ks in sorted(grouped.items(), key=lambda x: -len(x[1])):
    print(f"{p}: {len(ks)} keys: {ks[:5]}")
