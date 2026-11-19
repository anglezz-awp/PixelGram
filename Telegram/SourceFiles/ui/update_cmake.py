with open("G:/TBuild/tdesktop/Telegram/CMakeLists.txt", "r", encoding="utf-8") as f:
    c = f.read()

target = "    ${src_loc}/../ThirdParty/material-color-utilities\n)"
replacement = """    ${src_loc}/../ThirdParty/material-color-utilities
)

# Fail build if m3_unmapped.txt is not empty
if (EXISTS "${src_loc}/ui/m3_unmapped.txt")
    file(READ "${src_loc}/ui/m3_unmapped.txt" m3_unmapped_data)
    string(STRIP "${m3_unmapped_data}" m3_unmapped_stripped)
    if (NOT m3_unmapped_stripped STREQUAL "")
        message(FATAL_ERROR "FATAL: m3_unmapped.txt is NOT empty! All tokens must be explicitly mapped: ${m3_unmapped_stripped}")
    endif()
endif()"""

if target in c:
    c = c.replace(target, replacement, 1)
    with open("G:/TBuild/tdesktop/Telegram/CMakeLists.txt", "w", encoding="utf-8") as f:
        f.write(c)
    print("Updated CMakeLists.txt successfully!")
else:
    print("Target string not found in CMakeLists.txt!")
