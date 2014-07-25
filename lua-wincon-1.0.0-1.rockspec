package = "LuaWinCon"

version = "1.0.0-1"

source = {
    url = "https://github.com/yasuoohno/luawincon/releases/download/v1.0.0-1/luawincon-1.0.0-1.zip"
}

description = {
    summary = "LuaWinCon - Windows Console Utility for Lua",
    detailed = [[
        WinCon is an add-on module that allows change colors for
        Windows console text and background.
    ]],
    homepage = "http://sceneryandfish.withnotes.net/?page_id=1954", 
    license = "MIT/X11",
    maintainer = "Yasuo Ohno <yasuo.ohno@gmail.com>"
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
