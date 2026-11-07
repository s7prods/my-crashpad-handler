@cl demo.cpp /EHsc /Fe:demo-debug /MTd /Zi /Od /D_DEBUG /D_UNICODE /DUNICODE /std:c++20 /link /SUBSYSTEM:CONSOLE /MANIFEST:EMBED /MACHINE:X64 /DEBUG
@del /q *.obj
@del /q *.pdb
@del /q *.ilk
