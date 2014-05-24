/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <FLAC/stream_encoder.h>
#include <FLAC/metadata.h>
#include "xbmc/xbmc_audioenc_dll.h"
#include <string.h>

extern "C" {

int level=4;
FLAC__StreamEncoder *m_encoder=0;
FLAC__StreamMetadata *m_metadata[2];

static const int SAMPLES_BUF_SIZE = 1024 * 2;
FLAC__int32 *m_samplesBuf=0;
uint8_t* outbuffer=0;

uint8_t* m_tempBuffer=0; ///< temporary buffer

FLAC__StreamEncoderWriteStatus write_callback_flac(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame, void *client_data)
{
  if (outbuffer)
  {
    memcpy(outbuffer, buffer, bytes);
    outbuffer += bytes;
    return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
  }
  return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
}

//-- Create -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  return ADDON_STATUS_NEED_SETTINGS;
}

//-- Stop ---------------------------------------------------------------------
// This dll must cease all runtime activities
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Stop()
{
}

//-- Destroy ------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Destroy()
{
}

//-- HasSettings --------------------------------------------------------------
// Returns true if this add-on use settings
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
bool ADDON_HasSettings()
{
  return true;
}

//-- GetStatus ---------------------------------------------------------------
// Returns the current Status of this visualisation
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_GetStatus()
{
  return ADDON_STATUS_OK;
}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

//-- FreeSettings --------------------------------------------------------------
// Free the settings struct passed from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------

void ADDON_FreeSettings()
{
}

//-- SetSetting ---------------------------------------------------------------
// Set a specific Setting value (called from XBMC)
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_SetSetting(const char *strSetting, const void* value)
{
  if (strcmp(strSetting,"level") == 0)
    level = *((int*)value);
  return ADDON_STATUS_OK;
}

//-- Announce -----------------------------------------------------------------
// Receive announcements from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
{
}

bool Init(int iInChannels, int iInRate, int iInBits,
          const char* title, const char* artist,
          const char* albumartist, const char* album,
          const char* year, const char* track, const char* genre,
          const char* comment, int iTrackLength)
{
  // we only accept 2 / 44100 / 16 atm
  if (iInChannels != 2 || iInRate != 44100 || iInBits != 16)
    return false;

  m_samplesBuf = new FLAC__int32[SAMPLES_BUF_SIZE];
  m_tempBuffer = new uint8_t[32768]; // 32k of buffer for metadata etc. at the start of the track

  // allocate libFLAC encoder
  m_encoder = FLAC__stream_encoder_new();
  if (!m_encoder)
  {
    return false;
  }

  FLAC__bool ok = 1;

  ok &= FLAC__stream_encoder_set_verify(m_encoder, true);
  ok &= FLAC__stream_encoder_set_channels(m_encoder, iInChannels);
  ok &= FLAC__stream_encoder_set_bits_per_sample(m_encoder, iInBits);
  ok &= FLAC__stream_encoder_set_sample_rate(m_encoder, iInRate);
  ok &= FLAC__stream_encoder_set_total_samples_estimate(m_encoder, (FLAC__uint64)iTrackLength * iInRate);
  ok &= FLAC__stream_encoder_set_compression_level(m_encoder, level);

  // now add some metadata
  FLAC__StreamMetadata_VorbisComment_Entry entry;
  if (ok)
  {
    if (
      (m_metadata[0] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT)) == NULL ||
      (m_metadata[1] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)) == NULL ||
      !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ARTIST", artist) ||
      !FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0], entry, false) ||
      !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ALBUM", album) ||
      !FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0], entry, false) ||
      !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ALBUMARTIST", albumartist) || 
      !FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0], entry, false) || 
      !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "TITLE", title) ||
      !FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0], entry, false) ||
      !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "GENRE", genre) ||
      !FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0], entry, false) ||
      !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "TRACKNUMBER", track) ||
      !FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0], entry, false) ||
      !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "DATE", year) ||
      !FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0], entry, false) ||
      !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "COMMENT", comment) ||
      !FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0], entry, false)
      ) 
    {
      ok = false;
    }
    else
    {
      m_metadata[1]->length = 4096;
      ok = FLAC__stream_encoder_set_metadata(m_encoder, m_metadata, 2);
    }
  }

  // initialize encoder in stream mode
  outbuffer = m_tempBuffer;
  if (ok)
  {
    FLAC__StreamEncoderInitStatus init_status;
    init_status = FLAC__stream_encoder_init_stream(m_encoder, write_callback_flac, NULL, NULL, NULL, 0);
    if (init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
    {
      ok = false;
    }
  }

  if (!ok)
  {
    return false;
  }

  return true;
}

int Encode(int nNumBytesRead, uint8_t* pbtStream, uint8_t* buffer)
{
  int out_bytes = 0;
  if (m_tempBuffer)
  { // copy our temp buffer across
    out_bytes = outbuffer - m_tempBuffer;
    memcpy(buffer, m_tempBuffer, out_bytes);
    delete[] m_tempBuffer;
    m_tempBuffer = NULL;
  }
  int nLeftSamples = nNumBytesRead / 2; // each sample takes 2 bytes (16 bits per sample)
  outbuffer = buffer;
  while (nLeftSamples > 0)
  {
    int nSamples = nLeftSamples > SAMPLES_BUF_SIZE ? SAMPLES_BUF_SIZE : nLeftSamples;

    // convert the packed little-endian 16-bit PCM samples into an interleaved FLAC__int32 buffer for libFLAC
    for (int i = 0; i < nSamples; i++)
    { // inefficient but simple and works on big- or little-endian machines.
      m_samplesBuf[i] = (FLAC__int32)(((FLAC__int16)(FLAC__int8)pbtStream[2*i+1] << 8) | (FLAC__int16)pbtStream[2*i]);
    }

    // feed samples to encoder
    if (!FLAC__stream_encoder_process_interleaved(m_encoder, m_samplesBuf, nSamples / 2))
    {
      return 0;
    }

    nLeftSamples -= nSamples;
    pbtStream += nSamples * 2; // skip processed samples
  }

  out_bytes += (outbuffer - buffer);
  outbuffer = 0;
  return out_bytes;
}

int Flush(uint8_t* buffer)
{
  outbuffer = buffer;
  FLAC__stream_encoder_finish(m_encoder);
  int outdata=outbuffer-buffer;
  outbuffer = 0;
  return outdata;
}

bool Close(const char* File)
{
  FLAC__bool ok = 0;

  if (m_encoder)
  {
    // now that encoding is finished, the metadata can be freed
    if (m_metadata[0])
      FLAC__metadata_object_delete(m_metadata[0]);
    if (m_metadata[1])
      FLAC__metadata_object_delete(m_metadata[1]);

    // delete encoder
    FLAC__stream_encoder_delete(m_encoder);
  }

  delete m_samplesBuf;

  return ok ? true : false;
}

}
