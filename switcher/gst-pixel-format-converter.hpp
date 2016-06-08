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

#ifndef __SWITCHER_GST_PIXEL_FORMAT_CONVERTER_H__
#define __SWITCHER_GST_PIXEL_FORMAT_CONVERTER_H__

#include <memory>
#include "./quiddity.hpp"
#include "switcher/gst-pipeliner.hpp"
#include "switcher/gst-shmdata-subscriber.hpp"
#include "switcher/shmdata-utils.hpp"

namespace switcher {
class GstPixelFormatConverter {
 public:
  using uptr = std::unique_ptr<GstPixelFormatConverter>;
  GstPixelFormatConverter(Quiddity* quid,
                          const char* property_name,
                          const char* display_text);
  GstPixelFormatConverter() = delete;
  ~GstPixelFormatConverter() = default;
  GstPixelFormatConverter(const GstPixelFormatConverter&) = delete;
  GstPixelFormatConverter& operator=(const GstPixelFormatConverter&) = delete;

  bool start(const std::string& shmpath_to_convert,
             const std::string& shmpath_converted);
  bool stop();

 private:
  Quiddity* quid_;
  // shmdata path
  std::string shmpath_to_convert_{};
  std::string shmpath_converted_{};
  // gst
  std::unique_ptr<GstPipeliner> gst_pipeline_;
  UGstElem shmsrc_{"shmdatasrc"};
  UGstElem queue_codec_element_{"queue"};
  UGstElem color_space_codec_element_{"videoconvert"};
  UGstElem capsfilter_{"capsfilter"};
  UGstElem shm_converted_{"shmdatasink"};
  static const size_t default_initial_shmsize_{67108864};
  std::unique_ptr<GstShmdataSubscriber> shmsrc_sub_{nullptr};
  std::unique_ptr<GstShmdataSubscriber> shmsink_sub_{nullptr};
  // custom properties:
  std::string prop_name_{};  // name to give to the prop to be installed
  Selection video_format_;
  PContainer::prop_id_t video_format_id_;

  std::vector<std::string> get_formats();
  bool disable_property();
  bool enable_property();
  std::string get_caps_str() const;
};

}  // namespace switcher
#endif
