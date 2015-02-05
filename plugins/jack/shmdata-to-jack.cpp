/*
 * This file is part of switcher-jack.
 *
 * switcher-myplugin is free software; you can redistribute it and/or
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

#include "switcher/gst-utils.hpp"
#include "switcher/scope-exit.hpp"
#include "./shmdata-to-jack.hpp"

namespace switcher {
SWITCHER_MAKE_QUIDDITY_DOCUMENTATION(ShmdataToJack,
                                     "Shmdata to Jack",
                                     "audio",
                                     "send audio data to jack",
                                     "LGPL",
                                     "jacksink2",
                                     "Nicolas Bouillot");
bool ShmdataToJack::init_gpipe() {
  if (!jack_client_) {
    g_warning("JackClient cannot be instancied");
    return false;
  }
  if (!make_elements())
    return false;
  init_startable(this);
  install_property(G_OBJECT(volume_), "volume", "volume", "Volume");
  install_property(G_OBJECT(volume_), "mute", "mute", "Mute");
  return true;
}

ShmdataToJack::ShmdataToJack(const std::string &name):
    jack_client_(name.c_str()),
    custom_props_(std::make_shared<CustomPropertyHelper>()) {
  jack_client_.set_jack_process_callback(&ShmdataToJack::jack_process,
                                         this);
}

int ShmdataToJack::jack_process (jack_nframes_t nframes, void *arg){
  //auto context = static_cast<ShmdataToJack *>(arg);
  //g_print("jack_process %u\n", jack_frame_time(context->jack_client_.get_raw()));
  // g_print("coucou");
  return 0;
}
void
ShmdataToJack::on_handoff_cb(GstElement */*object*/,
                             GstBuffer *buf,
                             GstPad *pad,
                             gpointer user_data) {
  ShmdataToJack *context = static_cast<ShmdataToJack *>(user_data);
  auto current_time = jack_frame_time(context->jack_client_.get_raw());
  GstCaps *caps = gst_pad_get_negotiated_caps(pad);
  if (nullptr == caps)
    return;
  On_scope_exit {gst_caps_unref(caps);};
  // gchar *string_caps = gst_caps_to_string(caps);
  // On_scope_exit {if (nullptr != string_caps) g_free(string_caps);};
  // g_print("on handoff, negotiated caps is %s\n", string_caps);
  const GValue *val =
      gst_structure_get_value(gst_caps_get_structure(caps, 0),
                              "channels");
  const int channels = g_value_get_int(val);
  // g_print("on handoff, channel number %d\n", channels);
  context->check_output_ports(channels);
  // HERE
  g_print("handoff %u\n", current_time);
  jack_nframes_t duration = GST_BUFFER_SIZE(buf)/(4*channels);
  context->drift_observer_.set_current_time_info(current_time, duration);
  // g_print("audio data buffer %p, size %d\n",
  //         GST_BUFFER_DATA(buf),
  //         GST_BUFFER_SIZE(buf));
}

void ShmdataToJack::check_output_ports(int channels){
  if (channels == channels_)
    return;
  channels_ = channels;
  jack_client_t *cl = jack_client_.get_raw();
  // thread safe operations on output ports
  { std::unique_lock<std::mutex> lock(output_ports_mutex_);
    // unregistering previous ports
    for (auto &it: output_ports_)
      jack_port_unregister(cl, it);
    // registering new ports
    for (int i = 0; i < channels; ++i)
      output_ports_.emplace_back(
          jack_port_register(cl,
                             std::string("output_" + std::to_string(i)).c_str(),
                             JACK_DEFAULT_AUDIO_TYPE,
                             JackPortIsOutput,
                             0));
  }  // unlocking output_ports_
}

bool ShmdataToJack::make_elements() {
  GError *error = nullptr;
  std::string description(std::string(" audioconvert ! audioresample ! volume ! ")
                          + " audioconvert ! "
                          + " capsfilter caps=\"audio/x-raw-float, "
                          "endianness=(int)1234, width=(int)32, rate="
                          + std::to_string(jack_client_.get_sample_rate())
                          + "\" !" 
                          + " fakesink silent=true signal-handoffs=true sync=false");
  GstElement *jacksink = gst_parse_bin_from_description(description.c_str(), TRUE, &error);
  if (error != nullptr) {
    g_warning("%s", error->message);
    g_error_free(error);
    return false;
  }
  g_object_set(G_OBJECT(jacksink), "async-handling", TRUE, nullptr);
  GstElement *volume =
      GstUtils::get_first_element_from_factory_name(GST_BIN(jacksink),
                                                    "volume");
  if (nullptr != volume_) {
    GstUtils::apply_property_value(G_OBJECT(volume_),
                                   G_OBJECT(volume),
                                   "volume");
    GstUtils::apply_property_value(G_OBJECT(volume_),
                                   G_OBJECT(volume),
                                   "mute");
  }
  if (handoff_handler_ > 0 && nullptr != fakesink_)
    g_signal_handler_disconnect(G_OBJECT(fakesink_),
                                handoff_handler_);
  GstElement *fakesink =
      GstUtils::get_first_element_from_factory_name(GST_BIN(jacksink),
                                                    "fakesink");
  handoff_handler_ = g_signal_connect(fakesink,
                                      "handoff",
                                      (GCallback)on_handoff_cb,
                                      this);
  if (nullptr != audiobin_) 
    GstUtils::clean_element(audiobin_);
  audiobin_ = jacksink;
  volume_ = volume;
  fakesink_ = fakesink;
  return true;
}

bool ShmdataToJack::start() {
  if (false == make_elements())
    return false;
  set_sink_element(audiobin_);
  reinstall_property(G_OBJECT(volume_),
                     "volume", "volume", "Volume");
  reinstall_property(G_OBJECT(volume_),
                     "mute", "mute", "Mute");
  return true;
}

bool ShmdataToJack::stop() {
  if (!make_elements())
    return false;
  reset_bin();
  reinstall_property(G_OBJECT(volume_),
                     "volume", "volume", "Volume");
  reinstall_property(G_OBJECT(volume_),
                     "mute", "mute", "Mute");
  return true;
}

void ShmdataToJack::on_shmdata_disconnect() {
  stop();
}

void ShmdataToJack::on_shmdata_connect(std::string /* shmdata_sochet_path */ ) {
  if (is_started()) {
    stop();
    set_sink_element_no_connect(audiobin_);
  }
}

bool ShmdataToJack::can_sink_caps(std::string caps) {
  return GstUtils::can_sink_caps("audioconvert", caps);
}

}  // namespace switcher
