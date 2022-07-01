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
CXXFLAGS += -Wall -W -std=c++20 -Oz -fexceptions -sNO_DISABLE_EXCEPTION_CATCHING
LDFLAGS += -Llua/src -fexceptions -sEXPORTED_RUNTIME_METHODS=ccall,cwrap --shell-file shell.html --pre-js prologue.js --post-js epilogue.js --closure=1
LDLIBS += -llua

# closure compiler
# LDFLAGS += -Llua/src -fexceptions -sEXPORTED_RUNTIME_METHODS=ccall,cwrap --shell-file shell.html --closure=1

# source-map
# CXXFLAGS += -Wall -W -std=c++20 -g -fexceptions -sNO_DISABLE_EXCEPTION_CATCHING
# LDFLAGS += -Llua/src -fexceptions -gsource-map --source-map-base http://127.0.0.1/dromozoa-web/

OBJS = \
	boot.o \
	core.o \
	ffi.o
TARGET = index.html

all: all-recursive $(TARGET)

all-recursive:
	(cd lua/src && $(MAKE) CC=em++ AR="emar rcu" RANLIB=emranlib CFLAGS="-Wall -Wextra -DLUA_COMPAT_5_3 -Oz -fexceptions -sNO_DISABLE_EXCEPTION_CATCHING" MYLDFLAGS=-fexceptions LUA_T=lua.html LUAC_T=luac.html posix)

clean:
	$(RM) *.d *.o $(TARGET) index.html index.js index.wasm

clean-recursive: clean
	(cd lua/src && $(MAKE) clean && $(RM) *.d lua*.html lua*.js lua*.wasm)

$(TARGET): $(OBJS) shell.html prologue.js epilogue.js
	$(CXX) $(CPPFLAGS) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

.cpp.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<

-include $(OBJS:.o=.d)
