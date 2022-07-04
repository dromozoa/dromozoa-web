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

#ifndef DROMOZOA_WEB_ERROR_HPP
#define DROMOZOA_WEB_ERROR_HPP

#include <sstream>
#include <stdexcept>
#include <string>

namespace dromozoa {
  template <class... T>
  std::string make_error_impl(const char* file, int line, T&&... message) {
    std::ostringstream out;
    (out << ... << std::forward<T>(message)) << " at " << file << ":" << line;
    return out.str();
  }

  template <class T>
  class error : public T {
  public:
    template <class... U>
    error(const char* file, int line, U&&... message) : T(make_error_impl(file, line, std::forward<U>(message)...)) {}
  };
}

#define DROMOZOA_LOGIC_ERROR(...) \
  dromozoa::error<std::logic_error>(__FILE__, __LINE__, __VA_ARGS__) \
/**/

#define DROMOZOA_RUNTIME_ERROR(...) \
  dromozoa::error<std::runtime_error>(__FILE__, __LINE__, __VA_ARGS__) \
/**/

#endif
