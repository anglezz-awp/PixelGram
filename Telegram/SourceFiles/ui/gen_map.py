# -*- coding: utf-8 -*-
import re

with open("G:/TBuild/tdesktop/Telegram/SourceFiles/ui/all_keys.txt", "r", encoding="utf-8") as f:
    keys = [line.strip().split("\t")[0] for line in f if line.strip()]

print(f"Total keys to map: {len(keys)}")
