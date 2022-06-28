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

local bridge = require "dromozoa.web.bridge"

local window = bridge.get_window()
local document = window.document
print(window)
print(window.screenX, window.screenY)
print(window.no_such_key)
print(document)
print(document.location.href)
print(document.title)

document.title = "あいうえお"

print("document", document)
print("document.createElement", document.createElement)
local div = document.createElement(document, "div")
div:append "テストだよ"
div.id = "x"
div.style.color = "red"
div.style.backgroundColor = "black"
document.body:append(div)

local div2 = document:createElement("div")
div2.id = "y"
div2:append "これもテスト"
document.body:append(div2)

local div3 = document:getElementById "x"
print(div, div2, div3)
print("div==div2", div == div2)
print("div==div3", div == div3)
print("div==nil", div == nil)

div3:remove()
-- document.body:append(div3)

local df = document:createDocumentFragment()

print("!1", document:getElementById "x")
print("!2", df:getElementById "x")
print("!3", div3.isConnected)
print("!4", div3:getRootNode().nodeName)

df:append(div3)

print("!1", document:getElementById "x")
print("!2", df:getElementById "x")
print("!3", div3.isConnected)
print("!4", div3:getRootNode().nodeName)

document.body:append(df)

print("!1", document:getElementById "x")
print("!2", df:getElementById "x")
print("!3", div3.isConnected)
print("!4", div3:getRootNode().nodeName)

div3:addEventListener("click", function (ev)
  print(ev)
  print(ev.target.nodeName, ev.target.id)
end)

local link = document:createElement "a"
link.href = "index.html"
link:append "index.html"
div3:append(link)

link:addEventListener("click", function (ev)
  print "link clicked"
  ev:preventDefault()
  ev:stopPropagation()
end)

window.console:log "てすとだよ！！！！"

local div4 = document:createElement "div"
div4.id = "Z"
div4:append "removeのテスト"
div4:addEventListener("click", function (ev)
  print "div4 clicked"
end)
document.body:appendChild(div4)
-- document.body:removeChild(div4)
-- document.body:appendChild(div4)




