#include <stdio.h>
#include <string.h>
#include <pulse/pulseaudio.h>
// This test code show cases how an application can switch audio output between the two sinks and set the volume at leisure.
// It assumes that the system has more that one audio sinks available.  Ex:lineout and a bluetooth device.  

typedef struct
{
  int device_index;
  int volume;
}volume_info_struc_t;
volume_info_struc_t volume_info;

pa_context *volume_context;
pa_threaded_mainloop *pa_ml;
pa_mainloop_api *pa_mlapi;

int current_audio_index = 0;	//the current audio sink index;

void sink_input_cb(pa_context *c, const pa_sink_input_info *i, int eol, void *userdata);

//***************************************************************************************************
// This function sets the volume to the specified sink
// device_index : index of the sink
// volume : value of volume
// this method is called by your main application to set the volume of a particular sync 
// asynchronously
//***************************************************************************************************
void set_volume(int device_index, int volume) 
{
	if (device_index == -1) 
	{
		fprintf(stderr, "The requested device index %d is not configured on the system", device_index);
		return;
	}

	//always get sink input index since it can changes 
	pa_operation *o;
	volume_info.device_index = device_index;
	volume_info.volume = volume;

	//pa_context_get_sink_input_info_list - query the PA server for all the sinks presently connected to it. 
	//sink_input_cb is callback function triggered when PA has info available about any sink connected to the server
	//volume_info is a struct containing volume value to be applied. - pass it to the call back function
	if (!(o = pa_context_get_sink_input_info_list(volume_context, sink_input_cb, (void *) &volume_info))) 
	{
		fprintf(stderr, "pa_context sinkinput subscribe failed");
		return;
	}
	pa_operation_unref(o);
	return;
}

//***********************************************************************************************
void sink_input_cb(pa_context *c, const pa_sink_input_info *i, int eol, void *userdata) 
{

	char cmd[300];

	// If eol is set to a positive number, you're at the end of the list
	if (eol > 0) 
	{
	  return;
	}

	volume_info_struc_t *vol_info = (struct volume_info_struc_t *) userdata;
	fprintf(stdout, "sink input index : %d", (int )i->index);

	//the current sink is not the one whose volume change is requested
	if ( (i->index == current_audio_index) && (i->index != vol_info->device_index)) 
	{
		//moves the audio sink from current one to the one the user requested
		sprintf(cmd, "pacmd move-sink-input %d  %d", current_audio_index, vol_info->device_index);
		int status = system(cmd);
		if (status != 0) 
		{
			printf("unable to move sink input :  %d", status);
			return;
		} 
		else
		{
			printf("sink input changed");
			current_audio_index = vol_info->device_index;
		}

	}

	//set volume
	float calc_vol = vol_info->volume * 655.35;
	sprintf(cmd, "pacmd set-sink-volume %d  %d", vol_info->device_index, (int) calc_vol);

	int status = system(cmd); //drop system command into kernel.  change is effective immediately

}

//************************************************************************************************
// This is the context callback for setting volume
//************************************************************************************************
void volume_context_state_cb(pa_context *c, void *userdata) {

   switch (pa_context_get_state(c)) {

   case PA_CONTEXT_UNCONNECTED:
   case PA_CONTEXT_CONNECTING:
   case PA_CONTEXT_AUTHORIZING:
   case PA_CONTEXT_SETTING_NAME:
   break;

   case PA_CONTEXT_READY: {
     pa_operation *o;
   break;
   }
   case PA_CONTEXT_FAILED:
   case PA_CONTEXT_TERMINATED:
   default:
     return;

   }
}

void main(int argc, char*argv[])
{

  //create a mainloop API and connection to the default server
  //using a threaded loop because it's assumed that there is another mainloop for playback
  pa_ml = pa_threaded_mainloop_new();
  pa_mlapi = pa_threaded_mainloop_get_api(pa_ml);
  volume_context = pa_context_new(pa_mlapi, "Volume Control");

  //This function connects to the pulse server
  pa_context_connect(volume_context, NULL, 0, NULL);

  //This function defines a callback so the server will tell us its state
  pa_context_set_state_callback(volume_context, volume_context_state_cb, NULL);

  pa_threaded_mainloop_start(pa_ml);
  
}

