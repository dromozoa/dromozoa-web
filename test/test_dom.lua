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

local core = require "dromozoa.web.core"
local dom = require "dromozoa.web.dom"

local document <close> = dom.document()
local body <close> = document:query_selector "body"
local textarea <close> = document:query_selector "textarea"

print(textarea)
print(textarea.get_attribute_names)
local attribute_names = textarea:get_attribute_names()

print(attribute_names)
for i = 1, #attribute_names do
  local name = attribute_names[i]
  print(name, textarea:get_attribute(name))
end

print(textarea:get_attribute "no-such-attr")

attribute_names = nil

collectgarbage()
collectgarbage()

local div <close> = document:create_element "div"
div:append "テストだよ" :prepend "前に挿入"
body:append(div)

local s = core.get_now()
for i = 1, 240 do
  -- 4秒ごとに明滅する
  local t = core.get_now()
  local x = ((t - s) % 4000) / 2000 * math.pi
  local y = math.min(math.floor((math.cos(x) + 1) * 128), 255)
  local z = 255 - y

  textarea:set_attribute("style", ("background-color: #%02X%02X%02X; color: #FF%02X%02X"):format(z, z, z, y, y))
  coroutine.yield()
end

textarea:remove_attribute("style")

body:append(document:create_element "div" :append "おわったよ")

collectgarbage()
collectgarbage()
