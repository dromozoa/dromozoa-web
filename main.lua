R""--(

local dromozoa = require "dromozoa"

local coro = coroutine.create(function ()
  local function dump_fetch(key, fetch)
    print(key,
        fetch,
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

  local fetch2

  local fetch <close> = assert(dromozoa.fetch({
    request_method = "POST";
    attributes = dromozoa.fetch.LOAD_TO_MEMORY;
    onsuccess = function (fetch)
      fetch2 = fetch
      dump_fetch("onsuccess", fetch)
    end;
    onerror = function (fetch)
      dump_fetch("onerror", fetch)
    end;
    onprogress = function (fetch)
      dump_fetch("onprogress", fetch)
      -- error "die"
      -- fetch:close()
    end;
    request_headers = {
      "X-test1", 42;
      "X-test2", "foobarbaz";
      -- "X-test3", function () end;
    };
    request_data = "foo=bar&baz=qux";
  }, "main.lua?t=" .. os.time()))

  for i = 1, 10 do
    print(i)
    coroutine.yield()
  end

  print(fetch2)
  -- dump_fetch("main", fetch2)

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
