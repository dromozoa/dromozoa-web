# Copyright (C) 2022 Tomoyuki Fujimori <moyu@dromozoa.com>
#
# This file is part of dromozoa-web.
#
# dromozoa-png is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# dromozoa-png is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with dromozoa-png.  If not, see <http://www.gnu.org/licenses/>.

CXX = em++
CPPFLAGS += -Ilua/src -DLUA_USE_POSIX
CXXFLAGS += -Wall -W -O2 -fexceptions -sNO_DISABLE_EXCEPTION_CATCHING
LDFLAGS += -Llua/src -fexceptions
LDLIBS += -llua

OBJS = main.o
TARGET = main.html

all: $(TARGET)

clean:
	rm -f *.o $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CPPFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

.cpp.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<

