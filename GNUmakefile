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

BASE_CXXFALGS = -Oz -fexceptions -sNO_DISABLE_EXCEPTION_CATCHING
BASE_LDFLAGS = -fexceptions

CXX = em++
CPPFLAGS += -Ilua/src -DLUA_USE_POSIX -MMD
CXXFLAGS += -Wall -W -std=c++20 $(BASE_CXXFALGS)
LDFLAGS += -Llua/src $(BASE_LDFLAGS) -sEXPORTED_RUNTIME_METHODS=ccall,cwrap --shell-file shell.html --pre-js prologue.js --post-js epilogue.js
LDLIBS += -llua -lidbfs.js

ifneq ($(CLOSURE),)
LDFLAGS += --closure=$(CLOSURE)
endif

OBJS = \
	boot.o \
	error_queue.o \
	export.o \
	js_array.o \
	js_error.o \
	js_object.o \
	js_push.o \
	js_thread.o \
	module.o \
	runtime.o \
	utility.o
TARGET = index.html

all: all-recursive $(TARGET)

all-recursive:
	(cd lua/src && $(MAKE) CC=em++ AR="emar rcu" RANLIB=emranlib CFLAGS="-Wall -Wextra -DLUA_COMPAT_5_3 $(BASE_CXXFALGS)" MYLDFLAGS="$(BASE_LDFLAGS)" LUA_T=lua.html LUAC_T=luac.html posix)

clean:
	$(RM) *.d *.o $(TARGET) index.html index.js index.wasm

clean-recursive: clean
	(cd lua/src && $(MAKE) clean && $(RM) *.d lua*.html lua*.js lua*.wasm)

$(TARGET): $(OBJS) shell.html prologue.js epilogue.js
	$(CXX) $(CPPFLAGS) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

.cpp.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<

-include $(OBJS:.o=.d)
