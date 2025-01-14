/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 */

#ifndef _SHMDATA_UNIX_SOCKET_CLIENT_H_
#define _SHMDATA_UNIX_SOCKET_CLIENT_H_

#include <atomic>
#include <thread>
#include <string>
#include <condition_variable>
#include "./abstract-logger.hpp"
#include "./safe-bool-idiom.hpp"
#include "./unix-socket-protocol.hpp"
#include "./unix-socket.hpp"

namespace shmdata {

class UnixSocketClient : public SafeBoolIdiom {
 public:
  UnixSocketClient(const std::string& path, AbstractLogger* log);
  ~UnixSocketClient() override;
  UnixSocketClient() = delete;
  UnixSocketClient(const UnixSocketClient&) = delete;
  UnixSocketClient& operator=(const UnixSocketClient&) = delete;
  UnixSocketClient& operator=(UnixSocketClient&&) = delete;

  bool start(UnixSocketProtocol::ClientSide* proto);

 private:
  std::string path_;
  UnixSocket socket_;
  AbstractLogger* log_;
  std::thread socket_thread_{};
  std::atomic_short quit_{0};
  // connection
  std::mutex connected_mutex_{};
  std::condition_variable cv_{};
  std::atomic_bool connected_{false};
  std::atomic_bool is_valid_{false};
  UnixSocketProtocol::ClientSide* proto_{nullptr};
  bool is_valid() const final;
  void server_interaction();
};

}  // namespace shmdata
#endif
