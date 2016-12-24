#include <pebble.h>
#define MSG 0
#define DEBUG 0
#define CHIME 0
#define SECONDS 0
#define SECONDS_BARS 0

static Window *s_main_window; // Main window
static TextLayer *s_battery_status_layer;	// Battery status layer
static TextLayer *s_time_layer;	// Time layer
#if SECONDS
static TextLayer *s_seconds_layer; // Seconds layer
#endif
#if SECONDS_BARS
static TextLayer *s_seconds_bar_layer[5]; // Seconds bar layer
#endif
static TextLayer *s_today_date_layer; // Date layer
static TextLayer *s_today_date_underscore_layer; // Underscore below the date
#if MSG
static TextLayer *s_msg_layer;
#endif
static TextLayer *s_calendar_back_layer; // Calendar back layer
static TextLayer *s_calendar_days_layer[7]; // Calendar days layer

void deinit();
void init();
void main_window_load(Window *window);
void main_window_unload(Window *window);
void tick_handler(struct tm *tick_time, TimeUnits units_changed);
void update_battery_status(BatteryChargeState charge_state);
void update_calendar(); // Update the calendar layer
void update_date(struct tm *tick_time); // Update the date line layer
#if SECONDS
void update_seconds(struct tm *tick_time); // Update seconds
#endif
#if SECONDS_BARS
void update_seconds_bar(struct tm *tick_time); // Update seconds bars
#endif
void update_time(struct tm *tick_time); // Update the time layer
#if CHIME
void vibrate_chime();
#endif

int main(void){
	init();
	app_event_loop();
	deinit();
}

void deinit(){
	battery_state_service_unsubscribe();
	tick_timer_service_unsubscribe();
	window_destroy(s_main_window);
}

void init(){
	time_t tmp= time(NULL);
	struct tm *t= localtime(&tmp);
	// Create main Window element and assign to pointer
	s_main_window = window_create();
	window_set_background_color(s_main_window, GColorBlack);
	// Set handlers to manage the elements inside the Window
	window_set_window_handlers(s_main_window, (WindowHandlers) {
																	.load = main_window_load, .unload = main_window_unload
																}
							  );
	// Show the Window on the watch, with animated=false
	window_stack_push(s_main_window, false);
#if SECONDS_BARS || SECONDS
	tick_timer_service_subscribe(SECOND_UNIT, tick_handler); // Register with TickTimerService for seconds
#else
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler); // Register with TickTimerService for minutes
#endif
	battery_state_service_subscribe(update_battery_status); // Register with Battery State Service
	
	update_battery_status(battery_state_service_peek());
	update_time(t);
#if SECONDS
	update_seconds(t);
#endif
#if SECONDS_BARS
	update_seconds_bar(t);
#endif
	update_date(t);
	update_calendar();

	light_enable_interaction(); // Turn on the light when the watchface is started

}

void main_window_load(Window *window){
	int i;
	
	// Battery status layer
	s_battery_status_layer = text_layer_create(GRect(100, 0, 44, 15));
	text_layer_set_background_color(s_battery_status_layer, GColorClear);
	text_layer_set_text_color(s_battery_status_layer, GColorWhite);
	text_layer_set_font(s_battery_status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	text_layer_set_text_alignment(s_battery_status_layer, GTextAlignmentRight);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_battery_status_layer));
	
	// Time layer
	s_time_layer = text_layer_create(GRect(0, 15, 100, 30));
	text_layer_set_background_color(s_time_layer, GColorClear);
	text_layer_set_text_color(s_time_layer, GColorWhite);
	text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
	text_layer_set_text_alignment(s_time_layer, GTextAlignmentLeft);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
	
#if SECONDS
	// Seconds layer
	s_seconds_layer = text_layer_create(GRect(0, 31, 20, 15));
	text_layer_set_background_color(s_seconds_layer, GColorClear);
	text_layer_set_text_color(s_seconds_layer, GColorWhite);
	text_layer_set_font(s_seconds_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	text_layer_set_text_alignment(s_seconds_layer, GTextAlignmentRight);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_seconds_layer));
#endif
	
#if SECONDS_BARS
	// Seconds layer
	for(i = 0; i < 5; i++){
		s_seconds_bar_layer[i] = text_layer_create(GRect(142, 111 - (20 * i), 2, 10));
		text_layer_set_background_color(s_seconds_bar_layer[i], GColorClear);
		layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_seconds_bar_layer[i]));
	}
#endif
	
	// Date layer
	s_today_date_layer = text_layer_create(GRect(0, 45, 110, 18));
	text_layer_set_background_color(s_today_date_layer, GColorClear);
	text_layer_set_text_color(s_today_date_layer, GColorWhite);
	text_layer_set_font(s_today_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	text_layer_set_text_alignment(s_today_date_layer, GTextAlignmentLeft);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_today_date_layer));
	
	// Underscore layer
	s_today_date_underscore_layer = text_layer_create(GRect(0, 63, 100, 2));
	text_layer_set_background_color(s_today_date_underscore_layer, GColorWhite);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_today_date_underscore_layer));
	
#if MSG	
	// Msg layer
	s_msg_layer = text_layer_create(GRect(0, 65, 140, 18));
	text_layer_set_background_color(s_msg_layer, GColorClear);
	text_layer_set_text_color(s_msg_layer, GColorWhite);
	text_layer_set_font(s_msg_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	text_layer_set_text_alignment(s_msg_layer, GTextAlignmentLeft);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_msg_layer));
	text_layer_set_text(s_msg_layer, "This is a message.");
#endif
	
	// Calendar back layer
	s_calendar_back_layer = text_layer_create(GRect(0, 121, 144, 47));
	text_layer_set_background_color(s_calendar_back_layer, GColorWhite);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_calendar_back_layer)); 
	
	// Calendar days layers
	for(i = 0; i < 7; i++){
		s_calendar_days_layer[i] = text_layer_create(GRect(9+18*i, 121, 18, 47));
		text_layer_set_font(s_calendar_days_layer[i], fonts_get_system_font(FONT_KEY_GOTHIC_14));
		text_layer_set_text_alignment(s_calendar_days_layer[i], GTextAlignmentCenter);
		layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_calendar_days_layer[i])); 
	}
}

void main_window_unload(Window *window){
	int i;
#if DEBUG
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Begining unload.");
#endif
	for(i = 6; i >= 0; i--){
#if DEBUG
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Unloading s_calendar_days_layer[%d]", i);
#endif
		text_layer_destroy(s_calendar_days_layer[i]);
	}
#if DEBUG
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Finished s_calendar_days_layer unload");
#endif
	text_layer_destroy(s_calendar_back_layer);
#if MSG
	text_layer_destroy(s_msg_layer);
#endif
	text_layer_destroy(s_today_date_underscore_layer);
	text_layer_destroy(s_today_date_layer);
#if SECONDS
	text_layer_destroy(s_seconds_layer);
#endif
#if SECONDS_BARS
	for(i = 0; i < 5; i++){
#if DEBUG
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Unloading s_seconds_bar_layer[%d]", i);
#endif
		text_layer_destroy(s_seconds_bar_layer[i]);
	}
#endif
#if DEBUG
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Finished s_seconds_bar_layer unload");
#endif
	text_layer_destroy(s_time_layer);
	text_layer_destroy(s_battery_status_layer);
#if DEBUG
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Finished unload, destroying main window.");
#endif


}

void tick_handler(struct tm *tick_time, TimeUnits units_changed){

	if((tick_time->tm_sec % 10) == 0){
		if(tick_time->tm_sec == 0){
			update_time(tick_time); 
		}
#if SECONDS_BARS
		update_seconds_bar(tick_time);
#endif
		// Check if it's midnight to update the date and calendar
		if( (tick_time->tm_hour == 0) && (tick_time->tm_min == 0)){
			update_date(tick_time);
			update_calendar();
		}
	}
#if SECONDS
	update_seconds(tick_time);	
#endif
	
#if CHIME
	if((tick_time->tm_sec == 0) && (tick_time->tm_min == 0) && ((tick_time->tm_hour >= 7) && (tick_time->tm_hour <= 19))){
		vibrate_chime();		
	}
#endif
	
	
}

void update_battery_status(BatteryChargeState charge_state){ 
	static char buffer_battery[6];
	if(charge_state.is_charging == true){
		snprintf(buffer_battery, sizeof(buffer_battery), "+%d%%", charge_state.charge_percent);
	} else{
		snprintf(buffer_battery, sizeof(buffer_battery), "%d%%", charge_state.charge_percent);
	}
	text_layer_set_text(s_battery_status_layer, buffer_battery);
}

void update_calendar(){
	static char buffer_cal[7][9];
	time_t temp_time = time(NULL); 
	struct tm *today = localtime(&temp_time);
	int i;

	for(int i=0;i<7;i++){
		if(i==today->tm_wday){
			text_layer_set_background_color(s_calendar_days_layer[i], GColorBlack);
			text_layer_set_text_color(s_calendar_days_layer[i], GColorWhite);
		} else{
			text_layer_set_background_color(s_calendar_days_layer[i], GColorClear);
			text_layer_set_text_color(s_calendar_days_layer[i], GColorBlack);
		}
	}
	{
		buffer_cal[0][0] = 'S';
		buffer_cal[0][1] = 'u';
		buffer_cal[1][0] = 'M';
		buffer_cal[1][1] = 'o';
		buffer_cal[2][0] = 'T';
		buffer_cal[2][1] = 'u';
		buffer_cal[3][0] = 'W';
		buffer_cal[3][1] = 'e';
		buffer_cal[4][0] = 'T';
		buffer_cal[4][1] = 'h';
		buffer_cal[5][0] = 'F';
		buffer_cal[5][1] = 'r';
		buffer_cal[6][0] = 'S';
		buffer_cal[6][1] = 'a';
	}
	temp_time -= today->tm_wday*(60*60*24);
	today = localtime(&temp_time);
	for(i = 0 ;i < 7 ;i++){
		buffer_cal[i][2] = '\n';
		buffer_cal[i][5] = '\n';
		int day = today->tm_mday;
		if(day > 9){
			buffer_cal[i][3] = (day/10)+48;
		} else{
			buffer_cal[i][3] = '0';
		}
		buffer_cal[i][4] = (day%10)+48;
		temp_time += (7*60*60*24);
		today = localtime(&temp_time);
		day = today->tm_mday;
		if(day > 9){
			buffer_cal[i][6] = (day/10)+48;
		}else{
			buffer_cal[i][6] = '0';
		}
		buffer_cal[i][7] = (day%10)+48; 
		text_layer_set_text(s_calendar_days_layer[i],buffer_cal[i]);
		
		temp_time -= (6*60*60*24);
		today = localtime(&temp_time);
	}
	
}

void update_date(struct tm *tick_time){
	static char buffer_date[18];
	strftime(buffer_date, sizeof(buffer_date), "%A %b %d.", tick_time);
	text_layer_set_text(s_today_date_layer, buffer_date);
}

#if SECONDS
void update_seconds(struct tm *tick_time){
	static char buffer_seconds[4];
	
  	strftime(buffer_seconds, sizeof(buffer_seconds), ":%S", tick_time);
	text_layer_set_text(s_seconds_layer, buffer_seconds);
	
	GSize time_layer_size = text_layer_get_content_size(s_time_layer);
	GSize seconds_layer_size = text_layer_get_content_size(s_seconds_layer);
	seconds_layer_size.w = time_layer_size.w + seconds_layer_size.w;
	text_layer_set_size(s_seconds_layer, seconds_layer_size);
	
}
#endif

#if SECONDS_BARS
void update_seconds_bar(struct tm *tick_time){
	int bars = (int)(tick_time->tm_sec / 10);
	int i;
	for(i = 0; i < bars ; i++){
		text_layer_set_background_color(s_seconds_bar_layer[i], GColorWhite);
	}
	while(i < 5){
		text_layer_set_background_color(s_seconds_bar_layer[i], GColorClear);
		i++;
	}
}
#endif

void update_time(struct tm *tick_time){
	static char buffer_time[6];
	
	if(clock_is_24h_style() == true) {
  		strftime(buffer_time, sizeof(buffer_time), "%H:%M", tick_time);
 	} else{
  		strftime(buffer_time, sizeof(buffer_time), "%I:%M", tick_time);
	}
	text_layer_set_text(s_time_layer, buffer_time);
	
}

#if CHIME
void vibrate_chime(){
	vibes_short_pulse();
}
#endif