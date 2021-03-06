package = "lua-wincon"

version = "1.1.0-1"

source = {
    url = "https://github.com/yasuoohno/lua-wincon/releases/download/v1.1.0-1/lua-wincon-1.1.0-1.zip"
}

description = {
    summary = "lua-wincon - Windows Console Utility for Lua",
    detailed = [[
        lua-wincon is an add-on module that allows change colors for
        Windows console text and background.
    ]],
    homepage = "http://sceneryandfish.withnotes.net/?page_id=1954", 
    license = "MIT/X11",
    maintainer = "Yasuo Ohno (yasuo.ohno at gmail dot com)"
}

supported_platforms = {
    "windows"
}

dependencies = {
    "lua >= 5.1"
}

build = {
    type = "builtin",
    modules = {
        wincon = "src/lua_wincon.c"
    }
}
