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

local core = require "dromozoa.web.core"
local fetch = require "dromozoa.web.fetch"

local main_thread = coroutine.create(function ()
  local main_filename = "main.lua"
  local main_fetch
  local main_chunk

  local location_hash = core.run_script_string [[document.location.hash]]
  if location_hash:find "^#" then
    main_filename = location_hash:sub(2)
  end

  main_fetch = fetch({
    request_method = "GET";
    attributes = fetch.LOAD_TO_MEMORY;
    onsuccess = function (f)
      main_chunk = f:get_data()
      f:close()
      main_fetch = nil
    end;
    onerror = function (f)
      io.stdout:write(("fetch error: %s: %d %s\n"):format(main_filename, f:get_status(), f:get_status_text()))
      f:close()
      main_fetch = nil
    end;
  }, main_filename .. "?t=" .. os.time())

  while true do
    coroutine.yield()
    if not main_fetch then
      if not main_chunk then
        return
      end
      return assert(load(main_chunk, main_filename))()
    end
  end
end)

return function ()
  if not main_thread then
    return
  end

  local result, message = coroutine.resume(main_thread)
  if not result then
    io.stderr:write("main thread error: ", message, "\n")
  end
  if coroutine.status(main_thread) == "dead" then
    main_thread = nil
  end
end
--)"--"
