#!/bin/bash
em++ hash.cpp \
	-Os -flto -fno-exceptions -fno-rtti -DNDEBUG \
	-mtail-call -msimd128 \
	-sENVIRONMENT=web -sEXPORT_ES6=1 --no-entry \
	-sSTRICT=1 --closure 1 -sEXPORT_KEEPALIVE=1 \
	-sMINIMAL_RUNTIME=1 -sMINIMAL_RUNTIME_STREAMING_WASM_INSTANTIATION=1 \
	-sMALLOC=emmalloc -sINITIAL_HEAP=65536 \
	-sALLOW_MEMORY_GROWTH=1 -sMEMORY_GROWTH_LINEAR_STEP=65536 \
	-sEXPORTED_FUNCTIONS=_malloc -sEXPORTED_RUNTIME_METHODS=HEAPU8 \
	-o hash.js

perl -0777 -pi -e '
	my $arg;
	s|fetch\((new URL\([^)]+\))\)|$arg = $1; "req"|e;
	$_ = qq{let req=(u=>fetch(u,u.origin==location.origin?{mode:"no-cors",credentials:"include"}:void 0))($arg);\n$_};
' hash.js
