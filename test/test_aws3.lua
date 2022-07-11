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

local document = G.document
local FS = G.FS

local futures = { n = 0 }

function futures:async(fn)
  local n = self.n + 1
  self.n = n
  self[n] = async(fn)
end

local function element(source)
  -- { "name", attr = value, 1, 2, 3, 4 }
  local result = document:createElement(source[1])
  for k, v in pairs(source) do
    if type(k) == "string" then
      result:setAttribute(k, tostring(v))
    end
  end
  for i = 2, #source do
    local v = source[i]
    if type(v) == "table" then
      v = element(v)
    else
      v = tostring(v)
    end
    result:append(v)
  end
  return result
end

local function sync_fs()
  await(function (promise)
    FS:syncfs(function (e)
      promise:set(D.is_falsy(e), e)
    end)
  end)
  print "sync fs finished"
end

local credentials_path = "/save/credentials"

local function load_credentials()
  local access_key = ""
  local secret_key = ""

  local handle, message = io.open(credentials_path)
  if handle then
    access_key = handle:read "*l"
    secret_key = handle:read "*l"
    assert(handle:close())
  else
    io.stderr:write(("cannot open %s: %s\n"):format(credentials_path, message))
  end

  return access_key, secret_key
end

local function setup_credentials()
  local div = element { "div";
    { "div";
      { "label";
        "Access Key: ";
        { "input", name="access_key", type="text" };
      };
    };
    { "div";
      { "label";
        "Secret Key: ";
        { "input", name="secret_key", type="password" };
      };
    };
    { "div";
      { "button", name="save", "保存" };
      { "button", name="remove", "削除" };
    };
  }

  local access_key = div:querySelector "input[name=access_key]"
  local secret_key = div:querySelector "input[name=secret_key]"
  local save = div:querySelector "button[name=save]"
  local remove = div:querySelector "button[name=remove]"

  access_key.value, secret_key.value = load_credentials()

  save:addEventListener("click", function (ev)
    ev:preventDefault()
    ev:stopPropagation()
    local out = assert(io.open(credentials_path, "w"))
    out:write(access_key.value, "\n")
    out:write(secret_key.value, "\n")
    assert(out:close())
    futures:async(sync_fs)
  end)

  remove:addEventListener("click", function (ev)
    ev:preventDefault()
    ev:stopPropagation()
    local result, messaage = os.remove(credentials_path)
    if result then
      futures:async(sync_fs)
    else
      io.stderr:write(("cannot remove %s: %s\n"):format(credentials_path, message))
    end
  end)

  document.body:append(div)
end

local function setup_command(aws)
  local div = element { "div";
    { "div";
      { "label";
        "Path: ";
        { "input", name="path", type="text" };
      };
    };
    { "div";
      { "a", name="link", "(no data)" };
    };
    { "div";
      { "label";
        "File: ";
        { "input", name="file", type="file" };
      };
    };
    { "div";
      { "button", name="get", "GET" };
      { "button", name="put", "PUT" };
      { "button", name="delete", "DELETE" };
    };
  }

  local path = div:querySelector "input[name=path]"
  local link = div:querySelector "a[name=link]"
  local file = div:querySelector "input[name=file]"
  local get = div:querySelector "button[name=get]"
  local put = div:querySelector "button[name=put]"
  local delete = div:querySelector "button[name=delete]"

  get:addEventListener("click", function (ev)
    ev:preventDefault()
    ev:stopPropagation()
    futures:async(function ()
      local access_key, secret_key = load_credentials()
      local url = D.new(G.URL, "https://dromozoa-web.s3.ap-northeast-1.amazonaws.com")
      url.pathname = path.value
      local headers = aws.sign(access_key, secret_key, "GET", url, {})
      local response = await(G:fetch(url, { cache = "no-store", headers = headers }))
      if response.ok then
        local body = await(response:blob())
        local href = link:getAttribute "href"
        if D.is_truthy(href) then
          G.URL:revokeObjectURL(href)
        end
        link:setAttribute("href", G.URL:createObjectURL(body))
        link.textContent = body.type .. " " .. body.size
      else
        io.stderr:write(("cannot fetch %s: %d %s\n"):format(url, response.status, response.statusText))
      end
    end)
  end)

  put:addEventListener("click", function (ev)
    ev:preventDefault()
    ev:stopPropagation()
    futures:async(function ()
      local files = file.files
      if files.length == 0 then
        io.stderr:write "no file selected\n"
        return
      end
      local body = files[0]
      local access_key, secret_key = load_credentials()
      local url = D.new(G.URL, "https://dromozoa-web.s3.ap-northeast-1.amazonaws.com")
      url.pathname = path.value
      local headers = aws.sign(access_key, secret_key, "PUT", url, { ["content-type"] = body.type }, body)
      local response = await(G:fetch(url, { method = "PUT", cache = "no-store", headers = headers, body = body }))
      if response.ok then
        io.write(("%d %s\n"):format(response.status, response.statusText))
      else
        io.stderr:write(("cannot fetch %s: %d %s\n"):format(url, response.status, response.statusText))
      end
    end)
  end)

  delete:addEventListener("click", function (ev)
    ev:preventDefault()
    ev:stopPropagation()
    futures:async(function ()
      local access_key, secret_key = load_credentials()
      local url = D.new(G.URL, "https://dromozoa-web.s3.ap-northeast-1.amazonaws.com")
      url.pathname = path.value
      local headers = aws.sign(access_key, secret_key, "DELETE", url, {})
      local response = await(G:fetch(url, { method = "DELETE", cache = "no-store", headers = headers }))
      if response.ok then
        io.write(("%d %s\n"):format(response.status, response.statusText))
      else
        io.stderr:write(("cannot fetch %s: %d %s\n"):format(url, response.status, response.statusText))
      end
    end)
  end)

  document.body:append(div)
end

futures:async(function ()
  print("started", coroutine.running())
  local aws = async.require "dromozoa.web.aws"

  FS:mkdir "/save"
  FS:mount(G.IDBFS, {}, "/save")

  await(function (promise)
    FS:syncfs(true, function (e)
      promise:set(D.is_falsy(e), e)
    end)
  end)

  setup_credentials()
  setup_command(aws)

  print("finished", coroutine.running())
end)

while true do
  for k, future in pairs(futures) do
    if type(future) == "table" and future.is_ready and future:is_ready() then
      future:get()
      futures[k] = nil
    end
  end
  coroutine.yield()
end
