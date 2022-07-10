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

local D, G = require "dromozoa.web" .import "global"
local async, await = require "dromozoa.web.async" .import "await"

local document = G.document

local future = async(function ()
  local link = document:createElement "link"
    :setAttribute("href", "https://fonts.googleapis.com/css2?family=BIZ+UDPMincho&display=swap")
    :setAttribute("rel", "stylesheet")
  local ev = await(function (promise)
    link:addEventListener("load", function (ev)
      promise:set(true, ev)
    end)
    link:addEventListener("error", function (ev)
      promise:set(false, ev)
    end)
    document.head:append(link)
  end)

  local div = document:createElement "div"
  div.style.fontFamily = "'BIZ UDPMincho', serif"
  div:append(document:createElement "p"
    :append "昭和七十四年七月、ボクはキミに出逢った。人類が滅亡するまでの、最期のひとつきの、これは物語だ。")
  div:append(document:createElement "p"
    :append "破局から十年。現在の世界人口は約二十億人と推定されている。人類の生存圏は三十パーセントを下まわった。")
  document.body:append(div)

  print "finished"
end)

while true do
  if future and future:is_ready() then
    future:get()
    future = nil
  end
  coroutine.yield()
end
