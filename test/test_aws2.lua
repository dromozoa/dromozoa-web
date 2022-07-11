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

local futures = {}

local future = async(function ()
  -- https://docs.aws.amazon.com/AmazonS3/latest/API/sig-v4-header-based-auth.html
  local aws = async.require "dromozoa.web.aws"

  local document = G.document
  local access_key = document:createElement "input"
    :setAttribute("type", "text")
  local secret_key = document:createElement "input"
    :setAttribute("type", "password")
  local button = document:createElement "button"
    :append "テスト"

  button:addEventListener("click", function (ev)
    ev:preventDefault()
    ev:stopPropagation()

    futures[#futures + 1] = async(function ()
      local access_key = access_key.value
      local secret_key = secret_key.value
      print(access_key)
      print(secret_key)

      local url = "https://dromozoa-web.s3.ap-northeast-1.amazonaws.com/test.txt"
      local headers = aws.sign(access_key, secret_key, "GET", url, {})
      for i, item in D.each(headers:entries()) do
        local k, v = D.unpack(item)
        print(k, v)
      end

      local response = await(G:fetch(url, { cache = "no-store", headers = headers }))
      if not response.ok then
        error(("cannot fetch %s: %d %s"):format(filename, response.status, response.statusText))
      end
      local text = await(response:text())
      print(text)
      return text
    end)
  end)

  document.body:append(document:createElement "div"
    :append(document:createElement "div"
      :append(document:createElement "span" :append "access key:")
      :append(access_key))
    :append(document:createElement "div"
      :append(document:createElement "span" :append "private key:")
      :append(secret_key))
    :append(document:createElement "div"
      :append(button)))
end)

while true do
  if future then
    if future:is_ready() then
      future:get()
      future = nil
    end
  end

  for i = 1, #futures do
    local future = futures[i]
    if future then
      if future:is_ready() then
        future:get()
        futures[i] = false
      end
    end
  end

  coroutine.yield()
end
