#!/bin/bash
em++ hash.cpp \
	-O3 -flto -fno-exceptions -fno-rtti -DNDEBUG \
	-mtail-call -msimd128 -msse4.2 \
	-sENVIRONMENT=web -sEXPORT_ES6=1 --no-entry \
	-sSTRICT=1 --closure 1 -sEXPORT_KEEPALIVE=1 \
	-sMINIMAL_RUNTIME=1 -sSINGLE_FILE=1 \
	-sMALLOC=emmalloc -sINITIAL_HEAP=65536 \
	-sALLOW_MEMORY_GROWTH=1 -sMEMORY_GROWTH_LINEAR_STEP=65536 \
	-sEXPORTED_RUNTIME_METHODS=HEAPU8 \
	-o hash.js
