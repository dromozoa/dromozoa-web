R""--(

local dromozoa = require "dromozoa"

local coro = coroutine.create(function ()
  local function dump_fetch(key, fetch)
    print(key,
        fetch:get_url(),
        fetch:get_num_bytes(),
        fetch:get_total_bytes(),
        fetch:get_data_offset(),
        fetch:get_ready_state(),
        fetch:get_status(),
        fetch:get_status_text())
    -- print(fetch:get_data())
    -- print(fetch:get_data_pointer())
  end

  local fetch
  fetch = assert(dromozoa.fetch({
    request_method = "GET";
    attributes = dromozoa.fetch.LOAD_TO_MEMORY;
    onsuccess = function ()
      dump_fetch("onsuccess", fetch)
    end;
    onerror = function ()
      dump_fetch("onerror", fetch)
    end;
    onprogress = function ()
      dump_fetch("onprogress", fetch)
      fetch:close()
    end;
  }, "main.lua?t=" .. os.time()))

  print(fetch)
  dump_fetch("main", fetch)

  for i = 1, 10 do
    print(i)
    coroutine.yield()
  end

  fetch = nil

  collectgarbage()
  collectgarbage()
end)

return function ()
  if not coro then
    return
  end

  local result, message = coroutine.resume(coro)
  if not result then
    print("coroutine error: " .. message)
  end
  if coroutine.status(coro) == "dead" then
    coro = nil
  end
end

--)"--"
