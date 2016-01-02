//***********************************************************************************************
//***********************************************************************************************
// MAIN.C
// Phil T - 2015
//***********************************************************************************************
//***********************************************************************************************

//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
// LIBRARY HEADERS
//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
#include <pebble.h>

//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
// LOCAL HEADERS
//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
#include "main.h"
#include "watchface.h"

//===============================================================================================
// DATA
//===============================================================================================


//-----------------------------------------------------------------------------------------------
// STATIC / GLOBAL DATA
static Window *s_main_window;
//-----------------------------------------------------------------------------------------------



//===============================================================================================
// FUNCTIONS
//===============================================================================================

//-----------------------------------------------------------------------------------------------
// Tick Handler - Called every minute
//-----------------------------------------------------------------------------------------------
static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
	watchface_update_time( tick_time);
}

//-----------------------------------------------------------------------------------------------
// INIT FUNCTION - PEBBLE API REQUIREMENT
//-----------------------------------------------------------------------------------------------
static void init()
{

	// Create main Window element and assign to pointer
	s_main_window = window_create();
	


	// Set handlers to manage the elements inside the Window
	window_set_window_handlers(s_main_window, (WindowHandlers) 
	{
		.load = watchface_main_window_load,
		.unload = watchface_main_window_unload
	});

	
	
	// Show the Window on the watch, with animated=true
	window_stack_push(s_main_window, true);

	// Set the background colour, this saves doing it manually
	window_set_background_color(s_main_window, colWatchfaceBackgroundColour);
	
	// Register with TickTimerService
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

	

}

//-----------------------------------------------------------------------------------------------
// DEINIT FUNCTION - PEBBLE API REQUIREMENT
//-----------------------------------------------------------------------------------------------
static void deinit()
{
	// Destroy Window
	window_destroy(s_main_window);
}


//-----------------------------------------------------------------------------------------------
// MAIN FUNCTION - PEBBLE API REQUIREMENT
//-----------------------------------------------------------------------------------------------
int main(void) 
{
	init();
	app_event_loop();
	deinit();
}



