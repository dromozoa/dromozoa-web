R""--(

local dromozoa = require "dromozoa"

local coro = coroutine.create(function ()
  local metatable = getmetatable(dromozoa.fetch)
  print(metatable.__call)
  print(metatable.test)

  dromozoa.fetch()
  local u = 0
  for k, v in pairs(dromozoa.fetch) do
    print(k, v)
    u = u | v
  end
  print(("0x%0x"):format(u))

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
