== ENCODER PROFILE FORMAT ==

This documents the encoder profile format, for the video transcoders.  Note that
these profiles are gstreamer specific, and internal to the gstreamer+device
configurator.

The profile has a root element, <profile/>.
It has a number of attributes expressed as elements with text children:
  <type/> - the type of the encoder; should be "video"
  <id/> - a unique identifier.
  <priority/> - the base priority of the profile. (also set as when quality=0.5)
  <description/> - a short textual description of the profile.
  <mimetype/> - the mime type for the profile (unused?)
  <extension/> - the file extension for output files.
  
It also has a series of properties that depend on the quality setting input:
  <priority quality="n"/> - the text content is the priority at a given quality
    setting level.  The quality setting is a real number between 0 and 1.

There are also codec-related attributes of the profile:
  <contain/> - details about the muxer.
    <type/> - text content is the GStreamer (mime-ish) type of the container.
  
  <video/> - details about the video stream.
  <audio/> - details about the audio stream.

For <video/> and <audio/>, they have the following children:
  <type/> - text content is the GStreamer (mime-ish) type of the video stream
  <property/> - a property to set on the type; together, they specify what is
    effectively a capsfilter for the output.  <property/> has these attributes:
      name= - the name of the property to set on the GStreamer element.
      mapping= - (optional) if set, the value for this property will be taken
                 from the named one instead of the other attributes.
      type= - the type of the property; only "int" is really supported right now
      min=, max= - the ranges for the property
      default= - the default value for this property.
      scale= - a fractional scale that will be multiplied to the value before
               setting; used for things like converting between bps and kbps
      hidden= - if set, the property is not output; used for being the base for
                mappings for other properties.
  <quality-property/> - a property that is set based on the quality setting
                        (see <profile><priority quality=/>)
                        this has the given attributes:
    name= - see <property name=/>
    quality= - the quality input value
    value= - the value to set at the given quality leve.

  For video, the following quality-property names are special:
    bpp - the bits-per-pixel setting; used to derive a bitrate value based on
      the pixel dimensions of the output (and may affect the dimensions if it
      will be too big to fit the device output capabilities).  The value is a
      float, and will set bitrate= for use with <property mapping=/>.  Required.

  For audio, the following quality-property names are special:
    bitrate - the bitrate of the output. Required.
