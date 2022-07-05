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

local D = require "dromozoa.web"
local window = D.window
local document = window.document
local body = document.body

local df = document:createDocumentFragment()

local ul = document:createElement "ul"
for i, v in ipairs { "core", "fetch", "fetch2", "fetch3", "ffi", "file" } do
  local name = "test_" .. v
  ul:append(document:createElement "li"
    :append(document:createElement "a"
      :setAttribute("href", "?main=test/" .. name .. ".lua")
      :append(name)))
end
df:append(ul)

-- local div = document:createElement "div"
--   :setAttribute("style", "color: red")
-- div.innerHTML = [[<ol><li>あいうえお</li><li>かきくけこ</li><li>さしすせそ</li></ol>]]
-- df:append(div)

body:append(df)

while true do
  assert(D.get_error_queue())
  coroutine.yield()
end
