LuaWinCon
=========

Windows Console Utility for Lua

Overview
--------

This module allows some Windows console manipulation that change text and background colors and change codepage.

Usage
-----

    local wincon = require('wincon')
    --
    -- change console text color
    --
    wincon.SetTextColor(7, 1)
    print "Blue background"
    --
    -- return to the initial state (it is automatically done when the module detached from the process.)
    --
    wincon.SetTextAttribute(wincon.InitialTextAttribute())

Reference
---------

See [ldoc](doc/index.html)
