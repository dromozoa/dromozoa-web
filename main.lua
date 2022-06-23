R""--(

local dromozoa = require "dromozoa"

local fetch

local coro = coroutine.create(function ()
  fetch = assert(dromozoa.fetch({
    request_method = "GET";
    attributes = dromozoa.fetch.LOAD_TO_MEMORY;
    onsuccess = function ()
      print("success", fetch:get_ready_state(), fetch:get_status())
    end;
    -- onerror = function ()
    --   print("error", fetch:get_ready_state(), fetch:get_status())
    -- end;
    onprogress = function ()
      print("progress", fetch:get_ready_state(), fetch:get_status())
      fetch:close()
    end;
  }, "main.js?t=" .. os.time()))

  print(fetch)

  for i = 1, 10 do
    print(i)
    coroutine.yield()
  end

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
