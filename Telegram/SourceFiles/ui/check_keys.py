# -*- coding: utf-8 -*-
import re

with open("G:/TBuild/tdesktop/Telegram/SourceFiles/ui/all_keys.txt", "r", encoding="utf-8") as f:
    items = [line.strip().split("\t") for line in f if line.strip()]

print(f"Total keys: {len(items)}")
