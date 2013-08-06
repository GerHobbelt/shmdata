/*
 * Copyright (C) 2012-2013 Nicolas Bouillot (http://www.nicolasbouillot.net)
 *
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

#include "audio-source.h"
#include "gst-utils.h"
#include <memory>

namespace switcher
{
  AudioSource::AudioSource() 
  { 
    make_audio_elements ();
  }

  void 
  AudioSource::clean_audio_elements ()
  {
    //FIXME
    // GstUtils::clean_element(audio_tee_);
    // GstUtils::clean_element(audioconvert_);
    // GstUtils::clean_element(resample_);
    unregister_shmdata_writer (make_file_name ("audio"));
  }

  void 
  AudioSource::make_audio_elements ()
  {
   if (!GstUtils::make_element ("tee", &audio_tee_)
	|| !GstUtils::make_element ("audioconvert", &audioconvert_)
	|| !GstUtils::make_element ("audioresample", &resample_))
      g_debug ("a mandatory gst element is missing for audio source");


    gst_bin_add_many (GST_BIN (bin_),
		      audio_tee_,
		      audioconvert_,
		      resample_,
		      NULL);
    gst_element_link_many (audio_tee_,
			   audioconvert_,
			   resample_,
			   NULL);
  }


  void 
  AudioSource::set_raw_audio_element (GstElement *elt)
  {
    reset_bin ();
    clean_audio_elements ();
    make_audio_elements ();
    
    rawaudio_ = elt;
 
    gst_bin_add (GST_BIN (bin_), rawaudio_);
    gst_element_link (rawaudio_, audio_tee_);

    GstCaps *audiocaps = gst_caps_new_simple ("audio/x-raw-float",
					      "width", G_TYPE_INT, 32,
     					      NULL);

    //creating a connector for raw audio
    ShmdataWriter::ptr rawaudio_connector;
    rawaudio_connector.reset (new ShmdataWriter ());
    std::string rawconnector_name = make_file_name ("audio");
    rawaudio_connector->set_path (rawconnector_name.c_str());
    rawaudio_connector->plug (bin_, audio_tee_, audiocaps);
    register_shmdata_writer (rawaudio_connector);
    
    gst_caps_unref(audiocaps);

    GstUtils::wait_state_changed (bin_);
    GstUtils::sync_state_with_parent (rawaudio_);
    GstUtils::sync_state_with_parent (audio_tee_);
    GstUtils::sync_state_with_parent (audioconvert_);
    GstUtils::sync_state_with_parent (resample_);
  }

}
