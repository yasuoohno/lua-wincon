# Makefile for nmake.

T= wincon

#
# Lua modules installation directory
#
LUA_LIBDIR= "C:\lua\5.2"

#
# Lua files directory
#
LUA_INC= "C:\lua\5.2\include"
LUA_LIB= "C:\lua\5.2\lib\lua5.2.lib"

#
# module name
#
LIBNAME= src\$T.dll

INCS= /I$(LUA_INC)
WARN= /W3 /WX-
DBG= /Zi

CC= CL
CFLAGS= /nologo /MD /O2 /Oi /Oy- /Gm- /Gy /GS /fp:precise /GL /Gd $(WARN) $(DBG) $(INCS)
LD= LINK
LDFLAGS= /nologo /LTCG

SRCS=src\lua_$T.c
OBJS=src\lua_$T.obj

lib: $(LIBNAME)

.c.obj:
	$(CC) /c /Fo$@ $(CFLAGS) $<

$(LIBNAME): $(OBJS)
	$(LD) /DLL /OUT:$(LIBNAME) $(LDFLAGS) $(OBJS) $(LUA_LIB)
	IF EXIST $(LIBNAME).manifest mt -manifest $(LIBNAME).manifest -outputresource:$(LIBNAME);2

install: $(LIBNAME)
	IF NOT EXIST $(LUA_LIBDIR) mkdir $(LUA_LIBDIR)
	copy $(LIBNAME) $(LUA_LIBDIR)

clean:
	-@del /s *~ >NUL 2>NUL
	-@del vc110.pdb >NUL 2>NUL
	-@del $(LIBNAME) $(OBJS) src\$T.lib src\$T.exp >NUL 2>NUL
	-@del $(LIBNAME).manifest >NUL 2>NUL
