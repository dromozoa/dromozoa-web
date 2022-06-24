R""--(

-- local dromozoa = require "dromozoa"

local dw = {
  core = require "dromozoa.web.core";
  dom = require "dromozoa.web.dom";
  fetch = require "dromozoa.web.fetch";
}

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

  local fetch <close> = assert(dw.fetch({
    request_method = "POST";
    attributes = dw.fetch.LOAD_TO_MEMORY;
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

local coro = coroutine.create(function ()
  -- dw.core.run_script[[ alert("foo") ]]

  print(dw.core.run_script_string[[ document.location.href ]])
  print(dw.core.run_script_string[[ document.title ]])

  print(dw.core.get_device_pixel_ratio())
  print(dw.core.get_window_title())
  dw.core.set_window_title "あいうえお"

  print(dw.core.get_screen_size())

  for i = 1, 10 do
    print(i, dw.core.get_now(), dw.core.random())
    coroutine.yield()
  end
end)

local coro = coroutine.create(function ()
  local document = dw.dom.document()

  dw.core.run_script[[ console.log(dromozoa_web_dom.get(1)); ]]

  local element = document:query_selector ".not_found"
  print(element)

  local element = document:query_selector ".emscripten"
  print(element)

  local node_list = document:query_selector_all "div"
  print(node_list, #node_list)
  for i = 1, #node_list do
    local element = node_list[i]
    print(element)
  end

  document = nil
  collectgarbage()
  collectgarbage()

  dw.core.run_script[[ console.log(dromozoa_web_dom.get(1)); ]]

  for i = 1, 10 do
    print(i, dw.core.get_now(), dw.core.random())
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
