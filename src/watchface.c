//***********************************************************************************************
//***********************************************************************************************
// WATCHFACE.C
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
#include "watchface.h"

//===============================================================================================
// DATA
//===============================================================================================
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_status_layer;
static Layer *s_battery_layer;

static GFont s_custom_font_time;
static GFont s_custom_font_date;

static char s_time_buffer[6];
static char s_date_buffer[12];

static uint8_t u8CurrentMonth = 0xFF;
static uint8_t u8CurrentDay = 0xFF;

static bool bConnected = false;
static bool bCharging = false;
static bool bPlugged = false;


#define TIME_LAYER_HEIGHT 58
#define TIME_LAYER_TOP_PADDING -14

#define DATE_LAYER_HEIGHT 24
#define DATE_LAYER_BOTTOM_PADDING 2

#define STATUS_LAYER_HEIGHT 24
#define STATUS_LAYER_TOP_PADDING 4
#define STATUS_LAYER_BOTTOM_PADDING 4

#define BATTERY_LEVEL_HEIGHT 4
#define BATTERY_LEVEL_HORIZONTAL_PADDING 4
#define BATTERY_LEVEL_CHARGE_INDICATOR_WIDTH 4



//===============================================================================================
// FUNCTIONS
//===============================================================================================
static void watchface_battery_update_proc(Layer *layer, GContext *ctx);
static void handle_battery(BatteryChargeState charge_state);
static void handle_bluetooth(bool connected);

//-----------------------------------------------------------------------------------------------
// Main Window Load
//-----------------------------------------------------------------------------------------------
void watchface_main_window_load(Window *window)
{

	// Get information about the Window
	Layer *window_layer = window_get_root_layer(window);
	GRect rectWindowBounds = layer_get_bounds(window_layer);
	
	//---------------------------------------
	// Define bounds for elements
	//---------------------------------------
	// The battery level is a simple horixontal line accross the middle of the display
	GRect rectBatteryBounds = GRect( BATTERY_LEVEL_HORIZONTAL_PADDING , (rectWindowBounds.size.h - BATTERY_LEVEL_HEIGHT) / 2, (rectWindowBounds.size.w - (BATTERY_LEVEL_HORIZONTAL_PADDING * 2)) , BATTERY_LEVEL_HEIGHT);

	// The Data is above the battery level line
	GRect rectDateBounds = GRect(0, (rectBatteryBounds.origin.y - DATE_LAYER_HEIGHT - DATE_LAYER_BOTTOM_PADDING) , rectWindowBounds.size.w, DATE_LAYER_HEIGHT );
	
	// The time value is placed below the battery level line
	GRect rectTimeBounds = GRect(0, (rectBatteryBounds.origin.y + (BATTERY_LEVEL_HEIGHT + TIME_LAYER_TOP_PADDING)) , rectWindowBounds.size.w, TIME_LAYER_HEIGHT );
	
	
	// The status label is just below the date for a pebble round, or at the bottom for other pebbles.
	uint16_t u16YOffset = PBL_IF_ROUND_ELSE( (rectDateBounds.origin.y - STATUS_LAYER_HEIGHT + STATUS_LAYER_BOTTOM_PADDING) , (rectWindowBounds.size.h - STATUS_LAYER_HEIGHT - STATUS_LAYER_BOTTOM_PADDING));
	GRect rectStatusBounds = GRect(0, u16YOffset , rectWindowBounds.size.w , STATUS_LAYER_HEIGHT );
		
	//---------------------------------------
	// Create Bare Layers
	//---------------------------------------
	s_time_layer = text_layer_create( rectTimeBounds);
	s_date_layer =  text_layer_create( rectDateBounds);
	s_status_layer = text_layer_create(rectStatusBounds);
	s_battery_layer = layer_create(rectBatteryBounds);
	
		
								 
	// Load the custom fonts for the Time and Date
	s_custom_font_time = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_WATCHY_TIME_56));
	s_custom_font_date = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_WATCHY_DATE_20));
	
	
	//---------------------------------------
	// Time Text Layer
	//---------------------------------------
	
	text_layer_set_background_color(s_time_layer, colWatchfaceBackgroundColour);
	text_layer_set_text_color(s_time_layer, colWatchfaceTextColour);
	text_layer_set_font(s_time_layer, s_custom_font_time);
	text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

	// Add it as a child layer to the Window's root layer
	layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
	
	//---------------------------------------
	// Date Text Layer
	//---------------------------------------
	text_layer_set_background_color(s_date_layer, colWatchfaceBackgroundColour);
	text_layer_set_text_color(s_date_layer, colWatchfaceTextColour);
	text_layer_set_font(s_date_layer, s_custom_font_date);
	text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

	// Add it as a child layer to the Window's root layer
	layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

	//---------------------------------------
	// Status Text Layer
	//---------------------------------------
	text_layer_set_background_color(s_status_layer, colWatchfaceBackgroundColour);
	text_layer_set_text_color(s_status_layer, colWatchfaceTextColour);
	//text_layer_set_font(s_status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	text_layer_set_font(s_status_layer, s_custom_font_date);
	text_layer_set_text_alignment(s_status_layer, GTextAlignmentCenter);

	// Add it as a child layer to the Window's root layer
	layer_add_child(window_layer, text_layer_get_layer(s_status_layer));


	//---------------------------------------
	// Battery Layer
	//---------------------------------------
	layer_set_update_proc(s_battery_layer, watchface_battery_update_proc);
	layer_add_child(window_layer, s_battery_layer);
	
	
	//---------------------------------------
	// SUBSCRIBE TO ANY EVENTS
	//---------------------------------------
	battery_state_service_subscribe(handle_battery);
	
  	connection_service_subscribe((ConnectionHandlers) 
	{
    	.pebble_app_connection_handler = handle_bluetooth
  	});

	
	//---------------------------------------
	// UPDATE ALL ELEMENTS
	//---------------------------------------
	// Get a tm structure and update the time
	time_t temp = time(NULL);
	struct tm *tick_time = localtime(&temp);
	watchface_update_time(tick_time);
	
	
	handle_battery(battery_state_service_peek());
	handle_bluetooth(connection_service_peek_pebble_app_connection());
	
}

//-----------------------------------------------------------------------------------------------
// Main Window Unload
//-----------------------------------------------------------------------------------------------
void watchface_main_window_unload(Window *window)
{
	battery_state_service_unsubscribe();
	connection_service_unsubscribe();
		
	// Destroy TextLayer
	text_layer_destroy(s_time_layer);
	text_layer_destroy(s_date_layer);
	text_layer_destroy(s_status_layer);
	
	fonts_unload_custom_font(s_custom_font_time);
	fonts_unload_custom_font(s_custom_font_date);
	
	layer_destroy(s_battery_layer);
}


//-----------------------------------------------------------------------------------------------
// Update Time - Called very minute!
//-----------------------------------------------------------------------------------------------
void watchface_update_time(struct tm *tick_time) 
{

	// Now Write the current day and month into a buffer
	strftime(s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M" , tick_time);
	
	// Display this time on the date text layer
	text_layer_set_text(s_time_layer, s_time_buffer);
	
	// only update the date if it's changed!
	if( (tick_time->tm_mon != u8CurrentMonth) || (tick_time->tm_mday != u8CurrentDay))
	{
		// Now Write the current day and month into a buffer
		strftime(s_date_buffer, sizeof(s_date_buffer), "%a %d %b" , tick_time);
		// Display this time on the date text layer
		text_layer_set_text(s_date_layer, s_date_buffer);
		
		u8CurrentMonth = tick_time->tm_mon;
		u8CurrentDay = tick_time->tm_mday;
		
	}
	


}

//-----------------------------------------------------------------------------------------------
// Update Status - Aggregates all statuses for battery and bluetooth.
//-----------------------------------------------------------------------------------------------
static void update_status()
{
	

	if (bCharging)
	{
		text_layer_set_text(s_status_layer, "CHARGING");
	} 
	else if(bPlugged)
	{
		text_layer_set_text(s_status_layer, "CHARGED");
	}
	else if(!bConnected)
	{
		text_layer_set_text(s_status_layer, "DISCONNECTED");
	}
	else
	{
		text_layer_set_text(s_status_layer, "");
	}
		
	
}

//-----------------------------------------------------------------------------------------------
// Handle Battery - Called by the OS when battery status changes.
//-----------------------------------------------------------------------------------------------
static void handle_battery(BatteryChargeState charge_state) 
{
	bCharging = charge_state.is_charging;
	bPlugged = charge_state.is_plugged;
	if( (charge_state.charge_percent < 100) && bPlugged)
	{
		bCharging = true;
	}
	
	update_status();
}

//-----------------------------------------------------------------------------------------------
// Handle Bluetooth - Called by the OS when bluetooth connection changes.
//-----------------------------------------------------------------------------------------------
static void handle_bluetooth(bool connected) 
{
	bConnected = connected;
	if(!bConnected)
	{
		// Vibe pattern: ON for 200ms, OFF for 100ms, ON for 400ms:
		static const uint32_t const segments[] = { 200, 100, 200, 100, 200, 100, 200 };
		VibePattern pat = {
			.durations = segments,
			.num_segments = ARRAY_LENGTH(segments),
		};
		vibes_enqueue_custom_pattern(pat);
	}
	update_status();
}

//-----------------------------------------------------------------------------------------------
// Battery Update Proc - Called when the battery part needs drawing
//-----------------------------------------------------------------------------------------------
static void watchface_battery_update_proc(Layer *layer, GContext *ctx)
{
	// This is a simple paritally filled bubble according to battery level..
	BatteryChargeState sBatteryState = battery_state_service_peek();

	GRect rectLayerBounds = layer_get_bounds(layer);
	
	// Fill the entire layer in the foreground colour ()
	graphics_context_set_fill_color(ctx, colWatchfaceTextColour);
	graphics_fill_rect(ctx, rectLayerBounds,0,GCornerNone);
	
	// Now overlay a background coloured patch representing the current battery charge.
	// The width is static, so it will hollow out a small part of the bar
	int16_t i16StartX = (rectLayerBounds.size.w * sBatteryState.charge_percent) / 100;
	graphics_context_set_fill_color(ctx, colWatchfaceBackgroundColour);
	graphics_fill_rect(ctx, GRect(i16StartX, 1 , BATTERY_LEVEL_CHARGE_INDICATOR_WIDTH, rectLayerBounds.size.h - 2) , 0,GCornerNone);

}



