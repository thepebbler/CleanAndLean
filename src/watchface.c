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

#define TimeLayerHeight 60
#define DateLayerHeight 25

#define StatusLayerHeight 20
#define StatusLayerBottomPadding 2
#define BatteryStateDimension 15
#define BatteryStateInternalPadding 2
#define BatteryStateExternalPadding 2

#define BatteryStateTotalWidth (BatteryStateDimension + BatteryStateExternalPadding)

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
	GRect rectTimeBounds = GRect(0, (rectWindowBounds.size.h - (TimeLayerHeight + DateLayerHeight))  /2 , rectWindowBounds.size.w, TimeLayerHeight );
	GRect rectDateBounds = GRect(0, (rectTimeBounds.origin.y + TimeLayerHeight) , rectWindowBounds.size.w, DateLayerHeight );

	uint16_t u16YOffset = PBL_IF_ROUND_ELSE( (rectDateBounds.origin.y + DateLayerHeight + StatusLayerBottomPadding) , (rectWindowBounds.size.h - StatusLayerHeight - StatusLayerBottomPadding));

	GRect rectStatusBounds = GRect(BatteryStateTotalWidth, u16YOffset , (rectWindowBounds.size.w - (BatteryStateTotalWidth * 2)), StatusLayerHeight );
	
	
	uint16_t u16XOffset = PBL_IF_ROUND_ELSE( (rectWindowBounds.size.w - BatteryStateDimension) /2 , (rectWindowBounds.size.w - BatteryStateDimension - BatteryStateExternalPadding) );
	GRect rectBatteryBounds = GRect( u16XOffset , (rectWindowBounds.size.h - BatteryStateDimension - BatteryStateExternalPadding), BatteryStateDimension, BatteryStateDimension);
		
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
	//text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
	text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

	// Add it as a child layer to the Window's root layer
	layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
	
	//---------------------------------------
	// Date Text Layer
	//---------------------------------------
	text_layer_set_background_color(s_date_layer, colWatchfaceBackgroundColour);
	text_layer_set_text_color(s_date_layer, colWatchfaceTextColour);
	//text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	text_layer_set_font(s_date_layer, s_custom_font_date);
	//text_layer_set_text(s_date_layer,"Hello");
	text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

	// Add it as a child layer to the Window's root layer
	layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

	//---------------------------------------
	// Status Text Layer
	//---------------------------------------
	text_layer_set_background_color(s_status_layer, colWatchfaceBackgroundColour);
	text_layer_set_text_color(s_status_layer, colWatchfaceTextColour);
	text_layer_set_font(s_status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	//text_layer_set_font(s_status_layer, s_custom_font_date);
	text_layer_set_text(s_status_layer,"Hello");
	text_layer_set_text_alignment(s_status_layer, GTextAlignmentCenter);

	// Add it as a child layer to the Window's root layer
	layer_add_child(window_layer, text_layer_get_layer(s_status_layer));


	//---------------------------------------
	// Battery Layer
	//---------------------------------------
	layer_set_update_proc(s_battery_layer, watchface_battery_update_proc);
	layer_add_child(window_layer, s_battery_layer);
	
	// Get a tm structure and update the time
	time_t temp = time(NULL);
	struct tm *tick_time = localtime(&temp);
	watchface_update_time(tick_time);
	
	battery_state_service_subscribe(handle_battery);
	
  	connection_service_subscribe((ConnectionHandlers) 
	{
    	.pebble_app_connection_handler = handle_bluetooth
  	});
	
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

	// And Calculate the constraints of each object
	GPoint sMiddle = GPoint( (BatteryStateDimension / 2) , (BatteryStateDimension / 2) );
	const int16_t i16CircleRadius = (BatteryStateDimension / 2);
	int16_t i16EmptyHeight = 0;
	if(sBatteryState.charge_percent == 100)
	{
		i16EmptyHeight = 0;
	}
	else if(sBatteryState.charge_percent == 0)
	{
		i16EmptyHeight = BatteryStateDimension;
	}
	else
	{
		i16EmptyHeight = BatteryStateInternalPadding + (((BatteryStateDimension - BatteryStateInternalPadding) * (100 - sBatteryState.charge_percent)) / 100);
	}
		


	// First Draw the 'filled' circle
	// Draw a white filled circle a radius of half the layer height
	graphics_context_set_fill_color(ctx, GColorWhite);
	graphics_fill_circle(ctx, sMiddle, i16CircleRadius);

	// Then put a rectangle to erase the empty part
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_rect(ctx, GRect(0,0,BatteryStateDimension, i16EmptyHeight),0,GCornerNone);
	
	// Then draw an outline circle to complete it
	graphics_context_set_stroke_color(ctx, GColorWhite);
	graphics_draw_circle(ctx, sMiddle, i16CircleRadius);
	
	
}



