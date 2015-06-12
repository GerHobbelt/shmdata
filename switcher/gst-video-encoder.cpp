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

#include "switcher/std2.hpp"
#include "switcher/shmdata-utils.hpp"
#include "./gst-video-encoder.hpp"

namespace switcher {
SWITCHER_MAKE_QUIDDITY_DOCUMENTATION(
    GstVideoEncoder,
    "videnc",
    "Video Encoder",
    "video",
    "writer/reader",
    "Encode raw video stream",
    "LGPL",
    "Nicolas Bouillot");

GstVideoEncoder::GstVideoEncoder(const std::string &):
    shmcntr_(static_cast<Quiddity *>(this)),
    custom_props_(std::make_shared<CustomPropertyHelper>()),
    gst_pipeline_(std2::make_unique<GstPipeliner>(nullptr, nullptr)){
}

bool GstVideoEncoder::init() {
  codecs_ = std2::make_unique<GstVideoCodec>(static_cast<Quiddity *>(this),
                                             custom_props_.get(),
                                             std::string(),
                                             make_file_name("video-encoded"));
  shmcntr_.install_connect_method(
      [this](const std::string &shmpath){return this->on_shmdata_connect(shmpath);},
      [this](const std::string &){return this->on_shmdata_disconnect();},
      [this](){return this->on_shmdata_disconnect();},
      [this](const std::string &caps){return this->can_sink_caps(caps);},
      1);
  return true;
}

bool GstVideoEncoder::on_shmdata_disconnect() {
  return codecs_->stop();
}

bool GstVideoEncoder::on_shmdata_connect(const std::string &shmpath) {
  codecs_->set_shm(shmpath);
  return codecs_->start();
}

bool GstVideoEncoder::can_sink_caps(const std::string &caps) {
  // assuming codecs_ is internally using videoconvert as first caps negotiating gst element: 
  return GstUtils::can_sink_caps("videoconvert", caps);
}


// bool GstVideoEncoder::start() {
//   if (!gst_pipeline_)
//     return false;
//   shm_sub_ = std2::make_unique<GstShmdataSubscriber>(
//       shmdatasink_.get_raw(),
//       [this](std::string &&caps){
//         this->graft_tree(".shmdata.writer." + shmpath_,
//                          ShmdataUtils::make_tree(caps,
//                                                  ShmdataUtils::get_category(caps),
//                                                  0));
//       },
//       [this](GstShmdataSubscriber::num_bytes_t byte_rate){
//         this->graft_tree(".shmdata.writer." + shmpath_ + ".byte_rate",
//                          data::Tree::make(std::to_string(byte_rate)));
//       });
//   gst_pipeline_->play(true);
//   codecs_->start();
//   reinstall_property(G_OBJECT(videotestsrc_.get_raw()),
//                      "pattern", "pattern", "Video Pattern");
//   return true;
// }

// bool GstVideoEncoder::stop() {
//   shm_sub_.reset(nullptr);
//   prune_tree(".shmdata.writer." + shmpath_);
//   if (!UGstElem::renew(videotestsrc_, {"is-live", "pattern"})
//       || !UGstElem::renew(shmdatasink_, {"socket-path"})) {
//     g_warning("error initializing gst element for videotestsrc");
//     gst_pipeline_.reset();
//     return false;
//   }
//   gst_pipeline_ = std2::make_unique<GstPipeliner>(nullptr, nullptr);
//   gst_bin_add_many(GST_BIN(gst_pipeline_->get_pipeline()),
//                    shmdatasink_.get_raw(), videotestsrc_.get_raw(),
//                    nullptr);
//   gst_element_link(videotestsrc_.get_raw(), shmdatasink_.get_raw());
//   codecs_->stop();
//   reinstall_property(G_OBJECT(videotestsrc_.get_raw()),
//                      "pattern", "pattern", "Video Pattern");
//   return true;
// }

}  // namespace switcher
