-- Copyright (C) 2022 Tomoyuki Fujimori <moyu@dromozoa.com>
--
-- This file is part of dromozoa-web.
--
-- dromozoa-web is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- dromozoa-web is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with dromozoa-web.  If not, see <http://www.gnu.org/licenses/>.

local bridge = require "dromozoa.web.bridge"

local window = bridge.get_window()
local document = window.document
print(window)
print(window.screenX, window.screenY)
print(window.no_such_key)
print(document)
print(document.location.href)
print(document.title)

document.title = "あいうえお"

print("document", document)
print("document.createElement", document.createElement)
local div = document.createElement(document, "div")
div:append "テストだよ"
div.style.color = "red"
div.style.backgroundColor = "black"
document.body:append(div)
-- local div = document:createElement("div")

-- local window = bridge.get_window()
window.console:log "てすとだよ！！！！"

