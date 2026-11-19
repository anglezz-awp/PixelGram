with open("G:/TBuild/tdesktop/Telegram/SourceFiles/ui/chat/chat.style", "r", encoding="utf-8") as f:
    c = f.read()

c = c.replace("bubbleRadiusSmall: 6px;", "bubbleRadiusSmall: 4px;", 1)
c = c.replace("bubbleRadiusLarge: 18px;", "bubbleRadiusLarge: 20px;", 1)

with open("G:/TBuild/tdesktop/Telegram/SourceFiles/ui/chat/chat.style", "w", encoding="utf-8") as f:
    f.write(c)
print("Updated chat.style radii!")
