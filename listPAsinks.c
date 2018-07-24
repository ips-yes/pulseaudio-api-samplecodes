/* This notifies user when an audio sink is has been added or removed from the pulseaudio server */
/* This can be useful when writing an application that needs to detect when a new  */

#include <stdio.h>
#include <string.h>
#include <pulse/pulseaudio.h>

//Global variables
pa_context *connection_context;
void sink_subscribe_cb(pa_context *c, pa_subscription_event_type_t t, uint32_t index, void *userdata); 
static void sink_cb(pa_context *c, const pa_sink_info *i, int eol, void *userdata); 

void sink_context_state_cb(pa_context *c, void *userdata) 
{
    switch (pa_context_get_state(c)) 
    {
       case PA_CONTEXT_UNCONNECTED:
       case PA_CONTEXT_CONNECTING:
       case PA_CONTEXT_AUTHORIZING:
       case PA_CONTEXT_SETTING_NAME:
	   break;

       case PA_CONTEXT_READY: 
       {
	 pa_operation *o;

         //setup callback to tell us about sink devices
 	 if (! (o = pa_context_get_sink_info_list(c, sink_cb, userdata))) 
	 {
	    printf("pa_context_subscribe failed");
	    return;
	 }
	 pa_operation_unref(o);


	 pa_context_set_subscribe_callback(c, sink_subscribe_cb, userdata);
	
	 if (!(o = pa_context_subscribe(c, (pa_subscription_mask_t)
					  PA_SUBSCRIPTION_MASK_SINK, 
					  NULL, NULL)))
	 {
	   printf("pa_context_subscribe() failed");
	   return;
	 }
	 pa_operation_unref(o);
	
	 break;
       }

      case PA_CONTEXT_FAILED:
      case PA_CONTEXT_TERMINATED:
      default:
	 return;
    }
}




// call back to get info on sink. This saves sink info onto pulseaudio bulletin struct
static void sink_cb(pa_context *c, const pa_sink_info *i, int eol, void *userdata) 
{
    // If eol is set to a positive number, you're at the end of the list
    if (eol > 0) 
    {
	return;
    }
    
    if(i == NULL) { return;}

    printf("Sink Info: Name- %s, Description- %s Index- %zu", i->name, i->description, i->index);
}



void sink_subscribe_cb(pa_context *c, pa_subscription_event_type_t t, uint32_t index, void *userdata) 
{
   switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) 
   {

     case PA_SUBSCRIPTION_EVENT_SINK:

        if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE)
        {
	   //sink being removed from pulseaudio server
	   //this can be triggered when a bluetooth headset disconnects from the system
           printf("Removing sink index %d\n",index);
        }
        else
        {
	   //sink being added to pulseaudio server

	       pa_operation *o;
	       if (!(o = pa_context_get_sink_info_by_index(c, index, sink_cb, userdata))) //get more info about the added sink
	       {
		     printf("pa_context_get_sink_info_by_index() failed");
		     return;
	       }
	       pa_operation_unref(o);
	
	   
        }
        break;

   default:
      {
	     printf(" Other Sink Subscribe Event");
      }

   }

}



//*****************************************************************************************************
// This function registers a callback that updates bulletin structure with current pulseaudio sinks 
//*****************************************************************************************************
int main(int argc, char *argv[])
{
    int return_value;
    int userdata = 1;;
	
    pa_mainloop *mainloop;
    pa_mainloop_api *mainloop_api;

    // Create a mainloop API and connection to the default server
    
    mainloop = pa_mainloop_new(); 
    mainloop_api = pa_mainloop_get_api(mainloop);
    connection_context = pa_context_new(mainloop_api, "Device List");

    //This function connects to the pulse server
    pa_context_connect(connection_context, NULL, 0, NULL);

    //This function defines a callback so the server will tell us its state
    pa_context_set_state_callback(connection_context, sink_context_state_cb,(void *) userdata);

    if (pa_mainloop_run(mainloop, &return_value) < 0)
    {
	printf("pa_mainloop_run() failed.");
	exit(1);
    }
	

}

