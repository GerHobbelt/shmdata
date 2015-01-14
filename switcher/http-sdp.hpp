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

#ifndef __SWITCHER_HTTP_SDP_H__
#define __SWITCHER_HTTP_SDP_H__

#include <memory>
#include "./gst-pipeliner.hpp"

namespace switcher {
class HTTPSDP:public GstPipeliner {
 public:
  SWITCHER_DECLARE_QUIDDITY_PUBLIC_MEMBERS(HTTPSDP);
  HTTPSDP(const std::string &);
  ~HTTPSDP();
  HTTPSDP(const HTTPSDP &) = delete;
  HTTPSDP &operator=(const HTTPSDP &) = delete;

  bool to_shmdata(std::string uri);

 private:
  GstElement *souphttpsrc_{nullptr};
  GstElement *sdpdemux_{nullptr};
  int media_counter_{0};
  bool init_gpipe() final;
  static void pad_added_cb(GstElement * object,
                           GstPad *pad,
                           gpointer user_data);
  static gboolean to_shmdata_wrapped(gpointer uri,
                                     gpointer user_data);
  static void no_more_pads_cb(GstElement *object,
                              gpointer user_data);
};
}  // namespace switcher

#endif
