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
local async = require "dromozoa.web.async"
local await = async.await

local future = async(function ()
  local window = D.window
  local document = window.document

  local filename = "main.txt"
  local response = await(window:fetch(filename, { cache = "no-store" }))

  if not response.ok then
    error(("cannot fetch %s: %d %s"):format(filename, response.status, response.statusText))
  end

  local text = await(response:text())

  local ul = document:createElement "ul"
  for filename in text:gmatch "(.-)\n" do
    ul:append(document:createElement "li"
      :append(document:createElement "a"
        :setAttribute("href", "?dromozoa_web_main=" .. filename)
        :append(filename)))
  end
  document.body:append(ul)
end)

while true do
  if future and future:is_ready() then
    warn "@on"
    future:get(function (e) warn(tostring(e)) end)
    future = nil
  end

  assert(D.get_error_queue())
  coroutine.yield()
end
