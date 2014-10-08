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

#include <memory>
#include "./audio-source.hpp"
#include "./gst-utils.hpp"

namespace switcher {
AudioSource::AudioSource() :
    audio_tee_("tee") {
  make_audio_elements();
}

AudioSource::~AudioSource() {
}

void AudioSource::make_audio_elements() {
  UGstElem tmp("tee");
  if (!tmp)
    g_debug("cannot make tee for audio source");
  else
    audio_tee_ = std::move(tmp);
}

void AudioSource::set_raw_audio_element(GstElement *elt) {
  unset_raw_audio_element();
  make_audio_elements();
  rawaudio_ = elt;
  gst_bin_add_many(GST_BIN(bin_), rawaudio_, audio_tee_.get_raw(), nullptr);
  gst_element_link(rawaudio_, audio_tee_.get_raw());
  GstCaps *audiocaps = gst_caps_new_simple("audio/x-raw-int",
                                           "width",
                                           G_TYPE_INT, 16,
                                           nullptr);
  // creating a connector for raw audio
  ShmdataWriter::ptr shmdata_writer;
  shmdata_writer.reset(new ShmdataWriter());
  shmdata_path_ = make_file_name("audio");
  shmdata_writer->set_path(shmdata_path_.c_str());
  shmdata_writer->plug(bin_, audio_tee_.get_raw(), audiocaps);
  register_shmdata(shmdata_writer);
  gst_caps_unref(audiocaps);
  GstUtils::sync_state_with_parent(rawaudio_);
  GstUtils::sync_state_with_parent(audio_tee_.get_raw());
}

void AudioSource::unset_raw_audio_element() {
  if (!shmdata_path_.empty())
    unregister_shmdata(shmdata_path_);
  reset_bin();
}
}
