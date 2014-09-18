/*
 * This file is part of libswitcher.
 *
 * libswitcher is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __SWITCHER_RTPDESTINATION_H__
#define __SWITCHER_RTPDESTINATION_H__

/*
 * class meant to be possessed by and friend of RtpSession class.
 */

#include <gst/gst.h>
#include <map>
#include <string>
#include "./shmdata-reader.hpp"
#include "./quiddity-manager.hpp"
#include "./json-builder.hpp"

namespace switcher {
class RtpSession;
class RtpDestination {
 public:
  using ptr = std::shared_ptr<RtpDestination>;
  RtpDestination() = delete;
  RtpDestination(RtpSession *session); 
  ~RtpDestination();
  RtpDestination(const RtpDestination &) = delete;
  const RtpDestination &operator=(const RtpDestination &) = delete;
  void set_name(std::string name);
  void set_host_name(std::string host_name);
  std::string get_host_name();
  std::string get_port(std::string shmndata_path);
  // the reader of the rtp stream sent
  bool add_stream(std::string orig_shmdata_path,
                  QuiddityManager::ptr manager,
                  std::string port);
  bool has_shmdata(std::string shmdata_path);
  bool has_port(std::string port);
  bool remove_stream(std::string shmdata_stream_path);
  std::string get_sdp();
  // get json doc:
  JSONBuilder::Node get_json_root_node();

 private:
  std::string name_{};
  std::string host_name_{};
  // maps port with rtp shmdata reader:
  std::map<std::string, QuiddityManager::ptr> ports_{};
  // maps shmdata source stream with port:
  std::map<std::string, std::string> source_streams_{};
  JSONBuilder::ptr json_description_{};
  RtpSession *session_{nullptr};
  void make_json_description();
};
}  // namespace switcher

#endif                          // ifndef
