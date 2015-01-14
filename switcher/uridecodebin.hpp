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

#ifndef __SWITCHER_URIDECODEBIN_H__
#define __SWITCHER_URIDECODEBIN_H__

#include <unordered_map>
#include "./gst-pipeliner.hpp"
#include "./custom-property-helper.hpp"

namespace switcher {
class Uridecodebin:public GstPipeliner {
 public:
  SWITCHER_DECLARE_QUIDDITY_PUBLIC_MEMBERS(Uridecodebin);
  Uridecodebin(const std::string &);
  ~Uridecodebin();
  Uridecodebin(const Uridecodebin &) = delete;
  Uridecodebin &operator=(const Uridecodebin &) = delete;

 private:
  GstElement *uridecodebin_{nullptr};
  std::unordered_map<std::string, int>media_counters_{};
  GstPad *main_pad_{nullptr};
  GstCaps *rtpgstcaps_{nullptr};
  bool discard_next_uncomplete_buffer_{false};
  void init_uridecodebin();
  void destroy_uridecodebin();
  QuiddityCommand *on_error_command_{nullptr};  // for the pipeline error handler
  void clean_on_error_command();

  // custom properties
  CustomPropertyHelper::ptr custom_props_{};
  GParamSpec *loop_prop_{nullptr};
  bool loop_{false};
  GParamSpec *playing_prop_{nullptr};
  bool playing_{true};

  GParamSpec *uri_spec_{nullptr};
  std::string uri_{};

  bool init_gpipe() final;
  static gboolean get_loop(void *user_data);
  static void set_loop(gboolean mute, void *user_data);
  static void set_uri(const gchar *value, void *user_data);
  static const gchar *get_uri(void *user_data);
  bool to_shmdata();
  static void uridecodebin_pad_added_cb(GstElement * object, GstPad *pad,
                                        gpointer user_data);
  static gboolean to_shmdata_wrapped(gpointer uri, gpointer user_data);
  static void no_more_pads_cb(GstElement *object, gpointer user_data);
  static void source_setup_cb(GstElement *uridecodebin,
                              GstElement *source, gpointer user_data);
  static gboolean event_probe_cb(GstPad * pad, GstEvent *event,
                                 gpointer data);
  static gboolean process_eos(gpointer user_data);
  static void unknown_type_cb(GstElement * bin, GstPad *pad,
                              GstCaps *caps, gpointer user_data);
  static int autoplug_continue_cb(GstElement * bin, GstPad *pad,
                                  GstCaps *caps, gpointer user_data);
  static int autoplug_select_cb(GstElement * bin, GstPad *pad,
                                GstCaps *caps,
                                GstElementFactory *factory,
                                gpointer user_data);
  // filtering uncomplete custum buffers
  static gboolean gstrtpdepay_buffer_probe_cb(GstPad *pad,
                                              GstMiniObject *mini_obj,
                                              gpointer user_data);
  static gboolean gstrtpdepay_event_probe_cb(GstPad *pad,
                                             GstEvent *event,
                                             gpointer user_data);
  static void on_handoff_cb(GstElement *, GstBuffer *, GstPad *, gpointer);
  static void release_buf(void *);
  void pad_to_shmdata_writer(GstElement * bin, GstPad *pad);
  /* static GValueArray *autoplug_sort_cb (GstElement *bin, */
  /*  GstPad *pad, */
  /*  GstCaps *caps, */
  /*  GValueArray *factories, */
  /*  gpointer user_data); */
  /* static GValueArray *autoplug_factory_cb (GstElement *bin, */
  /*     GstPad *pad, */
  /*     GstCaps *caps, */
  /*     gpointer user_data); */
};
}  // namespace switcher

#endif                          // ifndef
