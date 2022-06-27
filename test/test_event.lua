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
local textarea = document:query_selector "textarea"

-- textarea:set_attribute("id", "ta")

print(document:query_selector "#output")

event.set_click_callback("#output", false, function ()
  print "click"
end)

