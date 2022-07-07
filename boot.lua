#if 0
--[[
#endif
"\n\n\n" R"]]--(
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

-- error
-- error "!!!"
-- error(D.null)

local thread
local future = async(function (self)
  local window = D.window
  local document = window.document

  local filename = D.new(window.URLSearchParams, document.location.search):get "main"
  if not filename or filename == D.null then
    filename = "main.lua"
  end

  local response = self:await(window:fetch(filename, { cache = "no-store" }))
  if not response.ok then
    error(("cannot fetch %s: %d %s"):format(filename, response.status, response.statusText))
  end

  local code = self:await(response:text())
  return coroutine.create(assert(load(code, "@" .. filename)))
end)

return function ()
  if future and future:is_ready() then
    thread = future:get()
    future = nil
  end

  if thread then
    assert(coroutine.resume(thread))
    if coroutine.status(thread) == "dead" then
      thread = nil
    end
  end
end

--)]]--"
