R""--(

local coro = coroutine.create(function ()
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
