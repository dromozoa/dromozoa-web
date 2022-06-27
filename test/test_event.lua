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
local event = require "dromozoa.web.event"

local document = dom.document()
local body = document:query_selector "body"
local textarea = document:query_selector "textarea"

-- textarea:set_attribute("id", "ta")

local div = document:create_element "div"
div:set_attribute("id", "mydiv")
div:append "CLICK ME!"
body:append(div)

local div = document:create_element "div"
div:set_attribute("id", "mydiv2")
div:append "CLICK YOU!"
body:append(div)


local function cb(event_type, event)
  print("click.output", event_type)
  for k, v in pairs(event) do
    print(k, v)
  end
end

event.set_click_callback("#output", false, cb)

local toggle = false

event.set_click_callback("#mydiv", false, function ()
  print "click.mydiv"
  toggle = not toggle
  if toggle then
    event.set_click_callback("#output", false)
  else
    event.set_click_callback("#output", false, cb)
  end
  local R = debug.getregistry()
  for k, v in pairs(R["dromozoa.web.event.callbacks"]) do
    print(k, v)
  end

  collectgarbage()
  collectgarbage()
end)

event.set_click_callback("#mydiv2", false, function ()
  print "click.mydiv2"
  event.set_click_callback("#mydiv2")
end)

