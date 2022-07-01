R""--(
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

local prototype = D.window.Promise.prototype
prototype.then_ = prototype["then"]

local boot_thread = coroutine.create(function ()
  local filename = D.new(D.window.URLSearchParams, D.window.document.location.search):get("main") or "main.lua"
  local chunk

  D.window:fetch(filename, { cache = "no-store" })
  :then_(function (response)
    return response:text()
  end)
  :then_(function (text)
    chunk = assert(load(text, filename))
  end)
  :catch(function (e)
    print("error ", e.message)
  end)

  while true do
    if chunk then
      break
    end
    coroutine.yield()
  end

  return chunk()
end)

return function ()
  if not boot_thread then
    return
  end

  local result, message = coroutine.resume(boot_thread)
  if not result then
    io.stderr:write("boot thread error: ", message, "\n")
  end
  if coroutine.status(boot_thread) == "dead" then
    boot_thread = nil
  end
end
--)"--"
