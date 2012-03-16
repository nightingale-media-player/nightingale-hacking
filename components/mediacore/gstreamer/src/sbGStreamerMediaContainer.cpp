/*
 *
 *=BEGIN SONGBIRD LICENSE
 *
 * Copyright(c) 2005-2011 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * For information about the licensing and copyright of this Add-On please
 * contact POTI, Inc. at customer@songbirdnest.com.
 *
 *=END SONGBIRD LICENSE
 *
 */

#include "sbGStreamerMediaContainer.h"
#include "sbGStreamerMediacoreUtils.h"
#include "sbIGStreamerService.h"
#include "sbProxiedComponentManager.h"

#include "nsThreadUtils.h"



NS_IMPL_ISUPPORTS1(sbGStreamerMediaContainer, sbIMediaContainer);



NS_IMETHODIMP
sbGStreamerMediaContainer::GetMimeType(nsACString & aMimeType)
{
  nsresult rv;

  // If no caps, then the MIME type is not yet acquired.
  if (!mCaps) {
    rv = AcquireMimeType_Priv();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ENSURE_TRUE(!mMimeType.IsEmpty(), NS_ERROR_NOT_AVAILABLE);
  aMimeType.Assign(mMimeType);

  return NS_OK;
}



NS_IMETHODIMP
sbGStreamerMediaContainer::GetPath(nsACString & aPath)
{
  aPath.Assign(mPath);
  return NS_OK;
}



NS_IMETHODIMP
sbGStreamerMediaContainer::SetPath(const nsACString & aPath)
{
  // Do nothing if the path is the same:
  if (mPath.Equals(aPath)) {
    return NS_OK;
  }

  // Clear the MIME type of the old file:
  mMimeType.Truncate();

  // Clear any caps of the old file:
  if (mCaps) {
    gst_caps_unref(mCaps);
    mCaps = NULL;
  }

  // Set the new path:
  mPath.Assign(aPath);

  return NS_OK;
}



sbGStreamerMediaContainer::~sbGStreamerMediaContainer ()
{
  // Dispose of any caps:
  if (mCaps)
  {
    gst_caps_unref(mCaps);
    mCaps = NULL;
  }
  
  // Dispose of the pipeline.  This unrefs all the pipeline elements:
  if (mPipeline) {
    gst_object_unref (mPipeline);
    mPipeline = NULL;
    mFilesrc = NULL;
    mTypefind = NULL;
    mSink = NULL;
  }

  // Dispose of the main loop:
  if (mLoop) {
    g_main_loop_unref (mLoop);
    mLoop = NULL;
  }
}



sbGStreamerMediaContainer::sbGStreamerMediaContainer () :
  mLoop(NULL),
  mPipeline(NULL),
  mFilesrc(NULL),
  mTypefind(NULL),
  mSink(NULL),
  mCaps(NULL)
{
}



nsresult
sbGStreamerMediaContainer::Init()
{
  // Initialize GStreamer.  Adapted from sbGStreamerPipeline.cpp
  // http://src.songbirdnest.com/xref/trunk/components/mediacore/gstreamer/src/sbGStreamerPipeline.cpp#87

  // We need to make sure the gstreamer service component has been loaded
  // since it calls gst_init for us.
  nsresult rv;
  nsCOMPtr<sbIGStreamerService> gstreamer;
  if (!NS_IsMainThread()) {
    gstreamer = do_ProxiedGetService(SBGSTREAMERSERVICE_CONTRACTID, &rv);
  }
  else {
    gstreamer = do_GetService(SBGSTREAMERSERVICE_CONTRACTID, &rv);
  }
  NS_ENSURE_SUCCESS(rv, rv);
  (void)gstreamer;

  // Create the loop in which the typefind pipeline will run:
  mLoop = g_main_loop_new (NULL, FALSE);
  NS_ENSURE_TRUE(mLoop, NS_ERROR_FAILURE);

  /* create a new pipeline to hold the elements */
  mPipeline = GST_PIPELINE (gst_pipeline_new ("pipe"));
  NS_ENSURE_TRUE(mPipeline, NS_ERROR_FAILURE);

  /* create file source and typefind element */
  mFilesrc = gst_element_factory_make ("filesrc", "source");
  NS_ENSURE_TRUE(mFilesrc, NS_ERROR_FAILURE);

  mTypefind = gst_element_factory_make ("typefind", "decoder");
  NS_ENSURE_TRUE(mTypefind, NS_ERROR_FAILURE);

  mSink = gst_element_factory_make ("fakesink", "sink");
  NS_ENSURE_TRUE(mSink, NS_ERROR_FAILURE);
  
  g_signal_connect (mTypefind, "have-type", G_CALLBACK(OnHaveType_Priv), this);
  
  /* setup */
  gst_bin_add_many (
      GST_BIN (mPipeline),
      mFilesrc,
      mTypefind,
      mSink,
      NULL);
  gst_element_link_many (
    mFilesrc,
    mTypefind,
    mSink,
    NULL);

  return NS_OK;
}



nsresult
sbGStreamerMediaContainer::AcquireMimeType_Priv()
{
  NS_ENSURE_TRUE(!mPath.IsEmpty(), NS_ERROR_NOT_INITIALIZED);

  if (mCaps) {
    // Already acquired
    return NS_OK;
  }

  // Set the bus message callback:
  NS_ENSURE_STATE(mPipeline);
  GstBus *  bus = gst_pipeline_get_bus (GST_PIPELINE (mPipeline));
  gst_bus_add_watch_full (
    bus,
    G_PRIORITY_DEFAULT,
    OnBusMessage_Priv,
    this,
    OnPrerollDone_Priv);
  
  // Set the filesrc target:
  NS_ENSURE_STATE(mFilesrc);
  g_object_set (G_OBJECT (mFilesrc), "location", mPath.get(), NULL);

  // Preroll the pipeline:
  NS_ENSURE_STATE(mLoop);
  gst_element_set_state (GST_ELEMENT (mPipeline), GST_STATE_PAUSED);
  g_main_loop_run (mLoop);
	
  // Get the MIME type if typefind got caps:
  if (mCaps) {
    // Check to see if there is video
    GstStructure *structure = gst_caps_get_structure (mCaps, 0);
    const gchar *name = gst_structure_get_name (structure);
    const bool isVideo = g_str_has_prefix (name, "video/");
    GetMimeTypeForCaps(mCaps, mMimeType, isVideo);
  }

  // Set the bus message callback for pipeline shutdown:
  gst_bus_add_watch_full (
    bus,
    G_PRIORITY_DEFAULT,
    OnBusMessage_Priv,
    this,
    OnPrerollDone_Priv);
  gst_object_unref (bus);

  // Shut down the pipeline:
  GstStateChangeReturn mode =
    gst_element_set_state (GST_ELEMENT (mPipeline), GST_STATE_NULL);
  if (mode == GST_STATE_CHANGE_ASYNC) {
    g_main_loop_run (mLoop);
  }
  
  // Return the proper result:
  NS_ENSURE_TRUE(!mMimeType.IsEmpty(), NS_ERROR_NOT_AVAILABLE);

  return NS_OK;
}



void
sbGStreamerMediaContainer::OnHaveType_Priv (
  GstElement *  typefind,
  guint         probability,
  GstCaps *     caps,
  gpointer      user_data)
{
  NS_ENSURE_TRUE(user_data,);
  sbGStreamerMediaContainer *	self =
    static_cast<sbGStreamerMediaContainer *>(user_data);

  // Save the found caps:
  self->mCaps = gst_caps_ref(caps);
}



gboolean
sbGStreamerMediaContainer::OnBusMessage_Priv (
  GstBus *		  bus,
  GstMessage *	message,
  gpointer		  data)
{
  NS_ENSURE_TRUE(data, FALSE);
  sbGStreamerMediaContainer *	self =
    static_cast<sbGStreamerMediaContainer *>(data);
  
  switch (GST_MESSAGE_TYPE (message))
  {
    case GST_MESSAGE_STATE_CHANGED:
    {
      GstState  from;
      GstState  to;
      GstState  goal;
      gst_message_parse_state_changed (message, &from, &to, &goal);
      
      GstObject * who = GST_MESSAGE_SRC (message);
      if (who == GST_OBJECT (self->mPipeline))
      {
        gboolean  done = (goal == GST_STATE_VOID_PENDING  ||  to == GST_STATE_NULL);
        return !done;
      }
    }
    break;
    
    case GST_MESSAGE_ERROR:
    {
      GError *  err;
      gchar *   debug;
      
      gst_message_parse_error(message, &err, &debug);
      
      g_error_free (err);
      g_free (debug);

      return FALSE;
    }
    break;
    
    case GST_MESSAGE_EOS:
    {
      return FALSE;
    }
    break;
    
    default:
    /* unhandled message */
    break;
  }
  
  
  return TRUE;
}


gboolean
sbGStreamerMediaContainer::OnMainLoopDone_Priv (
  gpointer		user_data)
{
  NS_ENSURE_TRUE(user_data, FALSE);
  sbGStreamerMediaContainer *	self =
    static_cast<sbGStreamerMediaContainer *>(user_data);
  g_main_loop_quit (self->mLoop);
  return FALSE;
}



void
sbGStreamerMediaContainer::OnPrerollDone_Priv (
  gpointer		user_data)
{
  g_idle_add (OnMainLoopDone_Priv, user_data);
}



sbGStreamerMediaContainer &
GetsbGStreamerMediaContainer()
{
  static sbGStreamerMediaContainer ssbGStreamerMediaContainer;

  return ssbGStreamerMediaContainer;
}
