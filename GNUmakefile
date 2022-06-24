# Copyright (C) 2022 Tomoyuki Fujimori <moyu@dromozoa.com>
#
# This file is part of dromozoa-web.
#
# dromozoa-web is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# dromozoa-web is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with dromozoa-web.  If not, see <http://www.gnu.org/licenses/>.

CXX = em++
CPPFLAGS += -Ilua/src -DLUA_USE_POSIX -MMD
CXXFLAGS += -Wall -W -std=c++17 -O2 -fexceptions -sNO_DISABLE_EXCEPTION_CATCHING
LDFLAGS += -Llua/src -fexceptions -sFETCH
LDLIBS += -llua

# source-map
# CXXFLAGS += -Wall -W -std=c++17 -g -fexceptions -sNO_DISABLE_EXCEPTION_CATCHING
# LDFLAGS += -Llua/src -fexceptions -sFETCH -gsource-map --source-map-base http://127.0.0.1/dromozoa-web/

OBJS = \
	core.o \
	exception_queue.o \
	fetch.o \
	main.o \
	module.o
TARGET = main.html

all: all-recursive $(TARGET)

all-recursive:
	(cd lua/src && $(MAKE) CC=em++ AR="emar rcu" RANLIB=emranlib MYCFLAGS="-fexceptions -sNO_DISABLE_EXCEPTION_CATCHING" MYLDFLAGS=-fexceptions LUA_T=lua.html LUAC_T=luac.html posix)

clean:
	$(RM) *.d *.o $(TARGET) main.html main.js main.wasm

clean-recursive:
	(cd lua/src && $(MAKE) clean && $(RM) lua*.html lua*.js lua*.wasm)

$(TARGET): $(OBJS)
	$(CXX) $(CPPFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

.cpp.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<

-include $(OBJS:.o=.d)
