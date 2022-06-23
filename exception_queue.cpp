// Copyright (C) 2022 Tomoyuki Fujimori <moyu@dromozoa.com>
//
// This file is part of dromozoa-web.
//
// dromozoa-web is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// dromozoa-web is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with dromozoa-web.  If not, see <http://www.gnu.org/licenses/>.

#include <deque>
#include <exception>
#include "exception_queue.hpp"

namespace dromozoa {
  namespace {
    std::deque<std::exception_ptr> exception_queue;
  }

  void push_exception_queue() {
    exception_queue.emplace_back(std::current_exception());
  }

  std::exception_ptr pop_exception_queue() {
    if (exception_queue.empty()) {
      return std::exception_ptr();
    } else {
      std::exception_ptr eptr = exception_queue.front();
      exception_queue.pop_front();
      return eptr;
    }
  }
}
