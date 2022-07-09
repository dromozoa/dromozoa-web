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

local D, G = require "dromozoa.web" .import "global"
local async, await = require "dromozoa.web.async" .import "await"

local future = async(function ()
  local filename = "main.txt"
  local response = await(G:fetch(filename, { cache = "no-store" }))
  if not response.ok then
    error(("cannot fetch %s: %d %s"):format(filename, response.status, response.statusText))
  end
  local data = await(response:text())
  local document = G.document
  local ul = document:createElement "ul"
  for filename in data:gmatch "(.-)\n" do
    ul:append(document:createElement "li"
      :append(document:createElement "a"
        :setAttribute("href", "?dromozoa_web_main=" .. filename)
        :append(filename)))
  end
  document.body:append(ul)
end)

while true do
  if future then
    if future:is_ready() then
      future:get()
      future = nil
    end
  end
  coroutine.yield()
end
