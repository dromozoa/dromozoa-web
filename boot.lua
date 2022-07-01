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
  local filename = D.new(D.window.URLSearchParams, D.window.document.location.search):get("main")
  if not filename or filename == D.null then
    filename = "main.lua"
  end
  local chunk
  local error_message

  D.window:fetch(filename, { cache = "no-store" })
  :then_(function (response)
    if response.ok then
      return response:text()
    else
      D.throw(D.new(D.window.Error, response.statusText))
    end
  end)
  :then_(function (text)
    chunk = assert(load(text, filename))
    done = true
  end)
  :catch(function (e)
    print("error ", e.message)
    done = true
    error_message = e.message
  end)

  while true do
    if done then
      break
    end
    coroutine.yield()
  end

  if chunk then
    return chunk()
  else
    error(error_message)
  end
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
