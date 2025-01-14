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

#include "./unix-socket-client.hpp"
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <unistd.h>

// OSX compatibility
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL SO_NOSIGPIPE
#endif

namespace shmdata {

UnixSocketClient::UnixSocketClient(const std::string& path, AbstractLogger* log)
    : path_(path), socket_(log), log_(log) {
  if (!socket_)  // client not valid if socket is not valid
    return;
  struct sockaddr_un sun;
  // fill socket address structure with server′s address
  memset(&sun, 0, sizeof(sun));
  sun.sun_family = AF_UNIX;
  strcpy(sun.sun_path, path.c_str());
  int len = offsetof(struct sockaddr_un, sun_path) + path_.size();
  if (0 != connect(socket_.fd_, (struct sockaddr*)&sun, len)) {
    int err = errno;
    if (ECONNREFUSED != err) log_->debug("connect: %", strerror(err));
    return;
  }
  is_valid_ = true;
}

UnixSocketClient::~UnixSocketClient() {
  if (is_valid_) {
    std::lock_guard<std::mutex> lock(connected_mutex_);
    if (socket_.fd_ != -1) {
      auto res = send(socket_.fd_, &proto_->quit_msg_, sizeof(proto_->quit_msg_), MSG_NOSIGNAL);
      if (-1 == res) log_->error("send (client trying to quit)");
    }
  }

  quit_.store(1);

  // if we didn't event start the thread, don't wait.
  if (socket_thread_.joinable()) {
    // now that we have stored quit, wait for the thread to finish.
    socket_thread_.join();
  }

}

bool UnixSocketClient::is_valid() const { return is_valid_; }

bool UnixSocketClient::start(UnixSocketProtocol::ClientSide* proto) {
  if (nullptr == proto) {
    log_->error("shmdata socket client needs a non null protocol");
    return false;
  }
  if (socket_thread_.joinable()) {
    log_->warning("shmdata socket client start has already invoked, ignoring");
    is_valid_ = false;
    return false;
  }
  proto_ = proto;
  socket_thread_ = std::thread([&]() { this->server_interaction(); });
  std::unique_lock<std::mutex> lock(connected_mutex_);
  cv_.wait_for(lock, std::chrono::milliseconds(1000), [&](){return connected_.load();});
  is_valid_ = is_valid_ && connected_;
  return connected_;
}

void UnixSocketClient::server_interaction() {
  fd_set allset;
  FD_ZERO(&allset);
  FD_SET(socket_.fd_, &allset);
  auto maxfd = socket_.fd_;
  struct timeval tv;  // select timeout
  bool quit = false;
  if (0 != quit_.load()) quit = true;
  bool quit_acked = false;
  while (!quit || !quit_acked) {
    // reset timeout since select may change values
    tv.tv_sec = 0;
    tv.tv_usec = 10000;  // 10 msec
    auto rset = allset;  /* rset gets modified each time around */
    if (select(maxfd + 1, &rset, NULL, NULL, &tv) < 0) {
      int err = errno;
      log_->error("select %", strerror(err));
      continue;
    }
    if (!FD_ISSET(socket_.fd_, &rset) && !connected_) {
      log_->debug("timed out");
    }
    if (FD_ISSET(socket_.fd_, &rset)) {
      ssize_t nread;
      if (!connected_) {
        nread = read(socket_.fd_, &proto_->data_, sizeof(UnixSocketProtocol::onConnectData));
      } else {
        std::lock_guard _{proto_->update_mtx_};
        nread = read(socket_.fd_, &proto_->update_msg_, sizeof(proto_->update_msg_));
      }
      if (nread <= 0) {
        if (nread < 0) {
          int err = errno;
          log_->error("read: %", strerror(err));
        }
        log_->debug("socket client, server error");
        if (connected_) proto_->on_disconnect_cb_();
        // disable socket
        FD_CLR(socket_.fd_, &allset);
        if (0 != close(socket_.fd_)) {
          int err = errno;
          log_->error("client closing socket %", strerror(err));
        }
        socket_.fd_ = -1;
        is_valid_ = false;
        quit = true;
        quit_acked = true;
      } else { /* process server′s message */
        if (!connected_) {
          // ack connection
          auto res = send(
              socket_.fd_, &proto_->data_, sizeof(UnixSocketProtocol::onConnectData), MSG_NOSIGNAL);
          if (-1 == res) {
            int err = errno;
            log_->error("client sending ack %", strerror(err));
          }
          proto_->on_connect_cb_();
          connected_ = true;
          log_->debug("client connected");
          std::lock_guard<std::mutex> lock(connected_mutex_);
          cv_.notify_one();
        } else {
          unsigned short msg_type{};
          size_t msg_size{};
          {
            std::lock_guard _{proto_->update_mtx_};
            msg_type = proto_->update_msg_.msg_type_;
            msg_size = proto_->update_msg_.size_;
          }
          if (1 == msg_type) {
            proto_->on_update_cb_(msg_size);
          } else if ((2 == msg_type)) {
            proto_->on_disconnect_cb_();
            log_->debug("client received quit");
            // disable socket
            std::lock_guard<std::mutex> lock(connected_mutex_);
            FD_CLR(socket_.fd_, &allset);
            if (0 != close(socket_.fd_)) {
              int err = errno;
              log_->error("client closing socket %", strerror(err));
            }
            socket_.fd_ = -1;
            is_valid_ = false;
            quit = true;
            quit_acked = true;
          }
        }
      }
    }
    if (0 != quit_.load()) quit = true;
  }
}

}  // namespace shmdata
