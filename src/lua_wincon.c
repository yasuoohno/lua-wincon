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

/**
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

/**
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

/**
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

/**
 * get background color from attribute.
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

/**
 * change background color on attribute.
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

/**
 * GetTextAttribute c function.
 *
 * Lua Usage:
 *   GetTextAttribute()
 *     : {number|nil} text attribute or nil if failed.
 *
 * @param L lua state
 * @return number of returned values.
 */
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

/**
 * SetTextAttribute c function.
 *
 * Lua Usage:
 *   SetTextAttribute(text_attribute)
 *     - {number} text_attribute
 *
 * @param L lua state
 * @return number of returned values.
 */
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

/**
 * GetTextColor lua c function.
 *
 * Lua Usage:
 *   GetTextColor()
 *     : {number|nil} foreground color number or nil if failed.
 *     : {number} background color number.
 *
 * @param L lua state
 * @return number of returned values.
 */
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

/**
 * SetTextColor lua c function.
 *
 * Lua Usage:
 *   SetTextColor(fore, back)
 *     - {number} fore - foreground color (0-15 or nil)
 *     - {number} back - background color (0-15 or nil)
 *     : {number} new text attribute.
 *
 * @param L lua state
 * @return number of returned values.
 */
static
int set_text_color(lua_State *L) {
    HANDLE hConsole;
    CONSOLE_SCREEN_BUFFER_INFO info;
    lua_Integer fore_color;
    lua_Integer back_color;
    WORD new_attribute;

    fore_color = -1;
    back_color = -1;

    if (lua_gettop(L) > 0 && ! lua_isnil(L, 1))
        fore_color = lua_tointeger(L, 1);
    if (lua_gettop(L) > 1 && ! lua_isnil(L, 2))
        back_color = lua_tointeger(L, 2);

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

/**
 * InitialTextAttribute lua c function.
 *
 * Lua Usage:
 *   InitialTextAttribute(new_attribute)
 *     - {number} new_attribute replace initial text attribute.
 *     : {number} current attribute
 *
 * @param L lua state
 * @return number of returned values.
 */
static
int initial_text_attribute(lua_State *L)
{
    if (lua_gettop(L) > 0)
        default_attribute = lua_tointeger(L, 1);

    lua_settop(L, 0);
    lua_pushinteger(L, default_attribute);
    return 1;
}

/**
 * GetCodepage lua c function.
 *
 * Lua Usage:
 *   GetCodepage()
 *     : {number} current input codepage.
 *
 * @param L lua state
 * @return number of returned values.
 */
static
int get_codepage(lua_State *L)
{
    UINT cp;

    cp = GetConsoleCP();

    lua_pushinteger(L, (lua_Integer)cp);
    return 1;
}

/**
 * SetCodepage lua c function.
 *
 * Lua Usage:
 *   SetCodepage(codepage)
 *     - codepage {number|string} input codepage.
 *     : {boolean} true if completed successfully.
 *
 * @param L lua state
 * @return number of returned values.
 */
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

/**
 * GetOutputCodepage lua c function.
 *
 * Lua Usage:
 *   GetOutputCodepage()
 *     : {number} current output codepage.
 *
 * @param L lua state
 * @return number of returned values.
 */
static
int get_output_codepage(lua_State *L)
{
    UINT cp;

    cp = GetConsoleOutputCP();

    lua_pushinteger(L, (lua_Integer)cp);
    return 1;
}

/**
 * SetOutputCodepage lua c function.
 *
 * Lua Usage:
 *   SetOutputCodepage(codepage)
 *     - codepage {number|string} output codepage.
 *     : {boolean} true if completed successfully.
 *
 * @param L lua state
 * @return number of returned values.
 */
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

/**
 * lua module entry point.
 *
 * @return number of returned values. always 1 (module table).
 */
int luaopen_wincon (lua_State *L) {
#if LUA_VERSION_NUM > 501
    luaL_newlib(L, funcs);
#else
    luaL_register(L, "wincon", funcs);
#endif
    return 1;
}
