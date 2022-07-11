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

local D, G = require "dromozoa.web" :import "global"
local async, await = require "dromozoa.web.async" :import "await"

local future = async(function ()
  local query = D.new(G.URLSearchParams, G.document.location.search)
  local filename = query:get "dromozoa_web_main"
  if D.is_falsy(filename) then
    filename = os.getenv "dromozoa_web_main"
  end
  if not filename then
    filename = "main.lua"
  end
  local response = await(G:fetch(filename, { cache = "no-store" }))
  if not response.ok then
    error(("cannot fetch %s: %d %s"):format(filename, response.status, response.statusText))
  end
  local code = await(response:text())
  return coroutine.create(assert(load(code, "@" .. filename)))
end)

local thread

return function ()
  assert(D.get_error_queue())
  async.dispatch()

  if future then
    if future:is_ready() then
      thread = future:get()
      future = nil
    end
  end

  if thread then
    local _, fn = assert(coroutine.resume(thread))
    if coroutine.status(thread) == "dead" then
      if fn then
        thread = coroutine.create(fn)
      else
        thread = nil
      end
    end
  end
end

--)]]--"
