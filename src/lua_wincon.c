///
/// module: wincon
/// release: 1.1.0-1
/// license: MIT/X11
/// author: Yasuo Ohno (yasuo.ohno at gmail dot com)
///

#include "lua_wincon.h"

#include <Windows.h>
#include "lauxlib.h"

const static WORD RESET_FOREGROUND_COLOR = ~FOREGROUND_BLUE & ~FOREGROUND_RED & ~FOREGROUND_GREEN & ~FOREGROUND_INTENSITY;
const static WORD RESET_BACKGROUND_COLOR = ~BACKGROUND_BLUE & ~BACKGROUND_RED & ~BACKGROUND_GREEN & ~BACKGROUND_INTENSITY;

/*
 * Text attribute when the dll has been attached.
 * This value will be restored when detached.
 */
static lua_Integer default_attribute = -1;

/*
 * dll entry point.
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    HANDLE hConsole;
    CONSOLE_SCREEN_BUFFER_INFO info;

    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hConsole != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(hConsole, &info) != 0)
            default_attribute = (lua_Integer)info.wAttributes;
        break;

    case DLL_PROCESS_DETACH:
        if (default_attribute != -1) {
            hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hConsole != INVALID_HANDLE_VALUE)
                SetConsoleTextAttribute(hConsole, (WORD)default_attribute);
        }
        break;

    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}

typedef struct _CPTable {
    const char *name;
    UINT code_page;
} CPTable;

static const CPTable cp_table[] = {
    { "ANSI",       0 },
    { "OEM",        1 },
    { "MAC",        2 },
    { "T_ANSI",     3 },
    { "SYMBOL",     4 },
    { "UTF7",   65000 },
    { "UTF8",   65001 },
    { NULL,         0 }
};

/*
 * convert string to codepage number.
 */
static
UINT string_to_cp(const char* str)
{
    int i;
    UINT cp;

    i = 0;
    while (cp_table[i].name != NULL) {
        if (_stricmp(cp_table[i].name, str) == 0) {
            cp = cp_table[i].code_page;
            break;
        }
        ++i;
    }

    // not found - convert string to number.
    if (cp_table[i].name == NULL)
        cp = (UINT)atoi(str);

    return cp;
}

/**
 * color table.
 */
static const char *color_table[] = {
    "BLACK",
    "NAVY",
    "MAROON",
    "PURPLE",
    "GREEN",
    "TEAL",
    "OLIVE",
    "GRAY",
    "BLACK",
    "BLUE",
    "RED",
    "FUCHSIA",
    "LIME",
    "AQUA",
    "YELLOW",
    "WHITE",
    NULL
};

/*
 * get foreground color from attribute
 */
static
lua_Integer get_foreground_color(WORD attribute) {
    lua_Integer color = 0;

    if (attribute & FOREGROUND_BLUE)
        color |= 1;
    if (attribute & FOREGROUND_RED)
        color |= 2;
    if (attribute & FOREGROUND_GREEN)
        color |= 4;
    if (attribute & FOREGROUND_INTENSITY)
        color |= 8;

    return color;
}

static
int is_decimal(const char *str)
{
    int result;

    result = 0;
    if (str == NULL || *str == 0)
        return 0;

    while(*str) {
        if (*str < '0' || *str > '9')
            return 0;
        ++str;
    }

    return 1;
}


/*
 * get color number from string
 */
static
lua_Integer string_to_color(const char* str)
{
    lua_Integer c;

    if (str == NULL)
        return (lua_Integer)-1;

    c = 0;
    while (color_table[c] != NULL) {
        if (_stricmp(color_table[c], str) == 0)
            return c;
        ++c;
    }

    // not found - convert string to number.
    if (is_decimal(str))
        c = (lua_Integer)atoi(str);

    if (c < 0 || c > 15)
        return (lua_Integer)-1;

    return c;
}

/*
 * get parameter as a color.
 */
static
lua_Integer get_color_param(lua_State *L, int index)
{
    lua_Integer color;

    // no parameter
    if (lua_gettop(L) < index)
        return -1;

    // nil parameter
    if (lua_isnil(L, index))
        return -1;

    // number or string
    if (lua_isnumber(L, index)) {
        color = lua_tointeger(L, index);
    } else if (lua_isstring(L, index)) {
        color = string_to_color(lua_tostring(L, index));
    } else {
        return luaL_error(L, "illegal color value");
    }

    if (color < 0 || color > 15)
        return luaL_error(L, "illegal color value.");

    return color;
}


/*
 * change foreground color on attribute
 */
static
WORD set_foreground_color(WORD attribute, lua_Integer color) {
    if (color < 0)
        return attribute;

    attribute &= RESET_FOREGROUND_COLOR;
    if (color & 1)
        attribute |= FOREGROUND_BLUE;
    if (color & 2)
        attribute |= FOREGROUND_RED;
    if (color & 4)
        attribute |= FOREGROUND_GREEN;
    if (color & 8)
        attribute |= FOREGROUND_INTENSITY;

    return attribute;
}

/*
 * get background color from an attribute.
 */
static
lua_Integer get_background_color(WORD attribute) {
    lua_Integer color = 0;

    if (attribute & BACKGROUND_BLUE)
        color |= 1;
    if (attribute & BACKGROUND_RED)
        color |= 2;
    if (attribute & BACKGROUND_GREEN)
        color |= 4;
    if (attribute & BACKGROUND_INTENSITY)
        color |= 8;

    return color;
}

/*
 * change background color on an attribute.
 */
static
WORD set_background_color(WORD attribute, lua_Integer color) {
    if (color < 0)
        return attribute;

    attribute &= RESET_BACKGROUND_COLOR;
    if (color & 1)
        attribute |= BACKGROUND_BLUE;
    if (color & 2)
        attribute |= BACKGROUND_RED;
    if (color & 4)
        attribute |= BACKGROUND_GREEN;
    if (color & 8)
        attribute |= BACKGROUND_INTENSITY;

    return attribute;
}

///
/// Get console text attribute.
///
/// Please refer to [Character Attributes](http://msdn.microsoft.com/en-us/library/windows/desktop/ms682088.aspx#_win32_character_attributes)
///
/// function: SetTextAttribute
/// tparam: number text attribute number.
///
static
int get_text_attribute(lua_State *L) {
    HANDLE hConsole;
    CONSOLE_SCREEN_BUFFER_INFO info;

    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE)
        return 0;

    if (GetConsoleScreenBufferInfo(hConsole, &info) == 0)
        return 0;

    lua_pushinteger(L, (lua_Integer)info.wAttributes);

    return 1;
}

///
/// Set console text attribute.
///
/// A simple wrapper of [SetConsoleTextAttribute](http://msdn.microsoft.com/en-us/library/windows/desktop/ms686047.aspx)
///
/// function: SetTextAttribute
/// tparam: number text attribute number.
///
static
int set_text_attribute(lua_State *L) {
    HANDLE hConsole;
    WORD new_attribute;

    if (lua_gettop(L) < 1 || lua_isnil(L, 1))
        return 0;

    new_attribute = (WORD)lua_tointeger(L, 1);

    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole != INVALID_HANDLE_VALUE)
        SetConsoleTextAttribute(hConsole, new_attribute);

    return 0;
}

///
/// Get text fore and back color.
///
/// function: GetTextColor
/// treturn: number foreground color number.
/// treturn: number background color number.
///
static
int get_text_color(lua_State *L) {
    HANDLE hConsole;
    CONSOLE_SCREEN_BUFFER_INFO info;
    WORD attribute;

    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE)
        return 0;
    if (GetConsoleScreenBufferInfo(hConsole, &info) == 0)
        return 0;

    attribute = info.wAttributes;
    lua_pushinteger(L, get_foreground_color(attribute));
    lua_pushinteger(L, get_background_color(attribute));

    return 2;
}

///
/// Set text fore and back color.
///
/// Color string/number list.
///
///   0. BLACK
///   1. NAVY
///   2. MAROON
///   3. PURPLE
///   4. GREEN
///   5. TEAL
///   6. OLIVE
///   7. GRAY
///   8. BLACK
///   9. BLUE
///   10. RED
///   11. FUCHSIA
///   12. LIME
///   13. AQUA
///   14. YELLOW
///   15. WHITE
///
/// function: SetTextColor
/// tparam: string|number|nil foreground
///  color string, number or nil.  
///  The foreground color will not be changed, if this parameter is nil.
/// tparam: string|number|nil background
///  color string, number or nil.  
///  The background color will not be changed, if this parameter is nil.
/// treturn: number|nil current text attribute number or nil if failed.
///
static
int set_text_color(lua_State *L) {
    HANDLE hConsole;
    CONSOLE_SCREEN_BUFFER_INFO info;
    lua_Integer fore_color;
    lua_Integer back_color;
    WORD new_attribute;

    fore_color = get_color_param(L, 1);
    back_color = get_color_param(L, 2);

    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE)
        return 0;
    if (GetConsoleScreenBufferInfo(hConsole, &info) == 0)
        return 0;

    new_attribute = set_foreground_color(info.wAttributes, fore_color);
    new_attribute = set_background_color(new_attribute, back_color);

    if (SetConsoleTextAttribute(hConsole, new_attribute) == 0)
        return 0;

    lua_pushinteger(L, (lua_Integer)info.wAttributes);

    return 1;
}

///
/// Get/Set initial text attribute.
///
/// Initial text attribute is used to restore text attribute when the dll is detaching from the process.  
/// Its default value is taken when the DLL was attaching.
///
/// function: InitialTextAttribute
/// tparam: ?number new_attribute text attribute number, -1 or nil(do not change).<br>The -1 value indicate that it will not restore text attribute when the dll is detaching.
/// treturn: number current initial text attribute number.
///
static
int initial_text_attribute(lua_State *L)
{
    if (lua_gettop(L) > 0)
        default_attribute = lua_tointeger(L, 1);

    lua_settop(L, 0);
    lua_pushinteger(L, default_attribute);
    return 1;
}

///
/// Get console input codepage
///
/// function: GetCodepage
/// treturn: number codepage number
///
static
int get_codepage(lua_State *L)
{
    UINT cp;

    cp = GetConsoleCP();

    lua_pushinteger(L, (lua_Integer)cp);
    return 1;
}

///
/// Set console input codepage
///
/// function: SetOutputCodepage
/// tparam: string|number codepage number or string.
///
/// If the codepage parameter is a string, it must be one of the followings.
///
///  * "ANSI"
///  * "OEM"
///  * "T_ANSI"
///  * "SYMBOL"
///  * "UTF7"
///  * "UTF8"
///
/// treturn: boolean|nil  
/// If the function succeeds, the return value is true.  
/// If the SetConsoleCP API failed, the return value is false. Otherwise nil.
///
static
int set_codepage(lua_State *L)
{
    UINT cp;

    // no parameter?
    if (lua_gettop(L) < 1)
        return 0;

    // switch parameter type.
    if (lua_isstring(L, 1)) {
        // string parameter.
        cp = string_to_cp(lua_tostring(L, 1));
    } else if (lua_isnumber(L, 1)) {
        // number parameter.
        cp = (UINT)lua_tointeger(L, 1);
    } else {
        // unsupported type.
        return 0;
    }

    lua_pushboolean(L, SetConsoleCP(cp) ? 1 : 0);
    return 1;
}

///
/// Get console output codepage
///
/// function: GetOutputCodepage
/// treturn: number codepage number
///
static
int get_output_codepage(lua_State *L)
{
    UINT cp;

    cp = GetConsoleOutputCP();

    lua_pushinteger(L, (lua_Integer)cp);
    return 1;
}

///
/// Set console output codepage
///
/// function: SetOutputCodepage
/// tparam: string|number codepage number or string.
///
/// If the codepage parameter is a string, it must be one of the followings.
///
///  * "ANSI"
///  * "OEM"
///  * "T_ANSI"
///  * "SYMBOL"
///  * "UTF7"
///  * "UTF8"
///
/// treturn: boolean|nil  
/// If the function succeeds, the return value is true.  
/// If the SetConsoleOutputCP API failed, the return value is false. Otherwise nil.
///
static
int set_output_codepage(lua_State *L)
{
    UINT cp;

    // no parameter?
    if (lua_gettop(L) < 1)
        return 0;

    // switch parameter type.
    if (lua_isstring(L, 1)) {
        // string parameter.
        cp = string_to_cp(lua_tostring(L, 1));
    } else if (lua_isnumber(L, 1)) {
        // number parameter.
        cp = (UINT)lua_tointeger(L, 1);
    } else {
        // unsupported type.
        return 0;
    }

    lua_pushboolean(L, SetConsoleOutputCP(cp) ? 1 : 0);
    return 1;
}

/**********************************************************************/

/*
 * export functions to lua.
 */
static struct luaL_Reg funcs[] = {
    { "GetTextAttribute", get_text_attribute },
    { "SetTextAttribute", set_text_attribute },
    { "GetTextColor", get_text_color },
    { "SetTextColor", set_text_color },
    { "InitialTextAttribute", initial_text_attribute },
    { "GetCodepage", get_codepage },
    { "SetCodepage", set_codepage },
    { "GetOutputCodepage", get_output_codepage },
    { "SetOutputCodepage", set_output_codepage },
    { NULL, NULL }
};

/*
 * lua module entry point.
 */
int luaopen_wincon (lua_State *L) {
#if LUA_VERSION_NUM > 501
    luaL_newlib(L, funcs);
#else
    luaL_register(L, "wincon", funcs);
#endif
    return 1;
}
