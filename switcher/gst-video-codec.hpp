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

#ifndef __SWITCHER_GST_VIDEO_CODEC_H__
#define __SWITCHER_GST_VIDEO_CODEC_H__

#include <vector>
#include "./default-video-format.hpp"

namespace switcher {
class quiddity;

class GstVideoCodec {
 public:
  GstVideoCodec(Quiddity *quid);
  GstVideoCodec();
  ~GstVideoCodec();
  GstVideoCodec(const GstVideoCodec &) = delete;
  GstVideoCodec &operator=(const GstVideoCodec &) = delete;

  void set_visible(bool visible);
  
 private:
  Quiddity *quid_;
  GstElement *rawvideo_{nullptr};
  GstElement *video_tee_{nullptr};
  std::string shmdata_path_{};
  // custom properties:
  CustomPropertyHelper::ptr custom_props_{};
  // codec // FIXME make this static
  GParamSpec *primary_codec_spec_{nullptr};
  GEnumValue primary_codec_[128]{};
  GParamSpec *secondary_codec_spec_{nullptr};
  GEnumValue secondary_codec_[128]{};
  gint codec_{0};
  // short or long codec list
  GParamSpec *codec_long_list_spec_{nullptr};
  bool codec_long_list_{false};
  GstElement *codec_element_{nullptr};
  GstElement *queue_codec_element_{nullptr};
  GstElement *color_space_codec_element_{nullptr};
  std::vector<std::string> codec_properties_{};
  DefaultVideoFormat::uptr video_output_format_{};
  
  virtual bool make_video_source(GstElement ** new_element) = 0;
  bool make_new_shmdatas();
  bool remake_codec_elements();
  void make_codec_properties();

  void show();
  void hide();

  static void set_codec(const gint value, void *user_data);
  static gint get_codec(void *user_data);
  // static gboolean get_codec_long_list(void *user_data);
  // static void set_codec_long_list(gboolean mute, void *user_data);
  static gboolean sink_factory_filter(GstPluginFeature *feature,
                                      gpointer data);
  static gint sink_compare_ranks(GstPluginFeature *f1,
                                 GstPluginFeature *f2);
  static gboolean reset_codec_configuration(gpointer /*unused */ , gpointer user_data);
};

}  // namespace switcher
#endif
