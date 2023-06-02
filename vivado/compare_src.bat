@echo off
set PATH=C:\Program Files\Beyond Compare 4;%PATH%

bcompare vivado\vivado_proj\system.tcl vivado\bd\system.tcl
bcompare vivado\vivado_proj\vivado_proj.srcs\constrs_1\imports\xdc\system.xdc vivado\xdc\system.xdc
