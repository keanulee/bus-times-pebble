#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "http.h"
#include "util.h"
#include "bus_times_pebble.h"

// POST keys
#define KEY_STOP_ID 0
#define KEY_LATITUDE 1
#define KEY_LONGITUDE 2
#define KEY_ACCURACY 3
#define KEY_PAGE 4
#define KEY_BUTTON 5

// Response keys
#define KEY_STOP_NAME_STR 0
#define KEY_DIRECTION_STR 1
#define KEY_TIME_STR 2

// Request IDs
#define HTTP_STOPS 2342
#define HTTP_ROUTE 2343

// Max number of nearby stops to receive
#define NUM_STOPS 10

// UI elements
Window window;
TextLayer layer_text0;
TextLayer layer_text1;
TextLayer layer_text2;
TextLayer layer_text3;
TextLayer layer_text4;

// UI strings
static char stop_name_str[20];
static char stop_id_str[8];
static char direction_str[24];
static char time_str[12];

// Keep track of which stop/route we're looking at
static int stop_ids[NUM_STOPS];
static int stop_count = 1;
static int stop_index;
static int route_index;

// Location information
static int our_latitude, our_longitude, our_accuracy;
static bool located;

// Flag to get routes information. Used to separate the requests for stops and routes
static bool get_routes;
static bool request_in_progress;

// This function is defined at the end of this file
void httpebble_error(int error_code);

void get_data_from_server(const int32_t request_id, int button)
{
	if (!located) {
		http_location_request();
		return;
	}

	if (!request_in_progress) {
		request_in_progress = true;

		DictionaryIterator* dict;
		HTTPResult result = http_out_get(URL, request_id, &dict);
		if (result != HTTP_OK) {
			httpebble_error(result);
			return;
		}

		dict_write_int32(dict, KEY_STOP_ID, stop_ids[stop_index]);
		dict_write_int32(dict, KEY_LATITUDE, our_latitude);
		dict_write_int32(dict, KEY_LONGITUDE, our_longitude);
		dict_write_int32(dict, KEY_ACCURACY, our_accuracy);
		dict_write_int32(dict, KEY_PAGE, route_index);
		dict_write_int32(dict, KEY_BUTTON, button);

		result = http_out_send();
		if (result != HTTP_OK) {
			httpebble_error(result); 
			return;
		}
	}
}

void up_single_click_handler(ClickRecognizerRef recognizer, Window *window)
{
	route_index--;
	get_data_from_server(HTTP_ROUTE, BUTTON_ID_UP);
}

void select_single_click_handler(ClickRecognizerRef recognizer, Window *window)
{
	stop_index = (stop_index + 1) % stop_count;
	memcpy(stop_id_str, itoa(stop_ids[stop_index]), 5);

	route_index = 0;

	get_data_from_server(HTTP_ROUTE, BUTTON_ID_SELECT);
}

void down_single_click_handler(ClickRecognizerRef recognizer, Window *window)
{
	route_index++;
	get_data_from_server(HTTP_ROUTE, BUTTON_ID_DOWN);
}

void click_config_provider(ClickConfig **config, Window *window) {
	config[BUTTON_ID_SELECT]->click.handler = (ClickHandler)select_single_click_handler;
	config[BUTTON_ID_UP]->click.handler     = (ClickHandler)up_single_click_handler;
	config[BUTTON_ID_DOWN]->click.handler   = (ClickHandler)down_single_click_handler;
}

void set_names()
{
	text_layer_set_text(&layer_text0, stop_name_str);
	text_layer_set_text(&layer_text1, stop_id_str);
	text_layer_set_text(&layer_text2, direction_str);
	text_layer_set_text(&layer_text3, time_str);
	text_layer_set_text(&layer_text4, AGENCY);
}

void http_success(int32_t request_id, int http_status, DictionaryIterator* received, void* context)
{
	request_in_progress = false;

	if (request_id == HTTP_STOPS) {
		stop_count = 0;
		for (int i = 0; i < NUM_STOPS; ++i) {
			Tuple* tuple = dict_find(received, i);
			if (tuple) {
				stop_ids[i] = tuple->value->int32;
				stop_count++;
			}
		}
		memcpy(stop_id_str, itoa(stop_ids[stop_index]), 5);
		get_routes = true;
	}
	if (request_id == HTTP_ROUTE) {
		Tuple* tuple0 = dict_find(received, KEY_STOP_NAME_STR);
		Tuple* tuple1 = dict_find(received, KEY_DIRECTION_STR);
		Tuple* tuple2 = dict_find(received, KEY_TIME_STR);
		memcpy(stop_name_str, tuple0->value->cstring, tuple0->length);
		memcpy(direction_str, tuple1->value->cstring, tuple1->length);
		memcpy(time_str, tuple2->value->cstring, tuple2->length);
		set_names();
	}
}

void http_failure(int32_t request_id, int http_status, void* context)
{
	httpebble_error(http_status >= 1000 ? http_status - 1000 : http_status);
}

void window_appear(Window* me)
{
	text_layer_set_text(&layer_text1, "Loading...");
	text_layer_set_text(&layer_text4, AGENCY);
}

void location(float latitude, float longitude, float altitude, float accuracy, void* context)
{
	// Fix the floats
	our_latitude  = latitude * 10000;
	our_longitude = longitude * 10000;
	our_accuracy  = accuracy;
	
	located = true;
	get_data_from_server(HTTP_STOPS, 0);
}

void handle_init(AppContextRef ctx)
{
	http_set_app_id(37629823);
	http_register_callbacks((HTTPCallbacks) {
		.success  = http_success,
		.failure  = http_failure,
		.location = location
	}, NULL);
	
	window_init(&window, AGENCY);
	window_stack_push(&window, true);
	window_set_window_handlers(&window, (WindowHandlers){
		.appear = window_appear
	});
	
	//Set the text boxes
	text_layer_init(&layer_text0, GRect(0, 0, 144, 30));
	text_layer_set_text_color(&layer_text0, GColorClear);
	text_layer_set_background_color(&layer_text0, GColorBlack);
	text_layer_set_font(&layer_text0, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	text_layer_set_text_alignment(&layer_text0, GTextAlignmentCenter);
	layer_add_child(&window.layer, &layer_text0.layer);

	text_layer_init(&layer_text1, GRect(0, 30, 144, 30));
	text_layer_set_text_color(&layer_text1, GColorClear);
	text_layer_set_background_color(&layer_text1, GColorBlack);
	text_layer_set_font(&layer_text1, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(&layer_text1, GTextAlignmentCenter);
	layer_add_child(&window.layer, &layer_text1.layer);

	text_layer_init(&layer_text2, GRect(0, 60, 144, 22));
	text_layer_set_text_color(&layer_text2, GColorBlack);
	text_layer_set_background_color(&layer_text2, GColorClear);
	text_layer_set_font(&layer_text2, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	text_layer_set_text_alignment(&layer_text2, GTextAlignmentCenter);
	layer_add_child(&window.layer, &layer_text2.layer);

	text_layer_init(&layer_text3, GRect(0, 82, 144, 32));
	text_layer_set_text_color(&layer_text3, GColorBlack);
	text_layer_set_background_color(&layer_text3, GColorClear);
	text_layer_set_font(&layer_text3, fonts_get_system_font(FONT_KEY_GOTHIC_28));
	text_layer_set_text_alignment(&layer_text3, GTextAlignmentCenter);
	layer_add_child(&window.layer, &layer_text3.layer);
	
	text_layer_init(&layer_text4, GRect(0, 125, 144, 32));
	text_layer_set_text_color(&layer_text4, GColorBlack);
	text_layer_set_background_color(&layer_text4, GColorClear);
	text_layer_set_font(&layer_text4, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	text_layer_set_text_alignment(&layer_text4, GTextAlignmentCenter);
	layer_add_child(&window.layer, &layer_text4.layer);

	graphics_draw_line(ctx, GPoint(0, 55), GPoint(144, 59));

	located = false;
	get_data_from_server(HTTP_STOPS, 0);

	window_set_click_config_provider(&window, (ClickConfigProvider) click_config_provider);
}

void handle_tick(AppContextRef ctxt, PebbleTickEvent *event)
{
	if (get_routes) {
		get_routes = false;
		get_data_from_server(HTTP_ROUTE, 0);
	}
}

void pbl_main(void *params)
{
	PebbleAppHandlers handlers = {
		.init_handler = &handle_init,
		.tick_info =
		{
			.tick_handler = &handle_tick,
			.tick_units = SECOND_UNIT
		},
		.messaging_info = {
			.buffer_sizes = {
				.inbound  = 256,
				.outbound = 256,
			}
		}
	};

	app_event_loop(params, &handlers);
}

void httpebble_error(int error_code)
{
	request_in_progress = false;

	static char error_message[] = "UNKNOWN_HTTP_ERRROR_CODE_GENERATED";
	switch (error_code) {
		case HTTP_SEND_TIMEOUT:
			strcpy(error_message, "HTTP_SEND_TIMEOUT");
		break;
		case HTTP_SEND_REJECTED:
			strcpy(error_message, "HTTP_SEND_REJECTED");
		break;
		case HTTP_NOT_CONNECTED:
			strcpy(error_message, "HTTP_NOT_CONNECTED");
		break;
		case HTTP_BRIDGE_NOT_RUNNING:
			strcpy(error_message, "HTTP_BRIDGE_NOT_RUNNING");
		break;
		case HTTP_INVALID_ARGS:
			strcpy(error_message, "HTTP_INVALID_ARGS");
		break;
		case HTTP_BUSY:
			strcpy(error_message, "HTTP_BUSY");
		break;
		case HTTP_BUFFER_OVERFLOW:
			strcpy(error_message, "HTTP_BUFFER_OVERFLOW");
		break;
		case HTTP_ALREADY_RELEASED:
			strcpy(error_message, "HTTP_ALREADY_RELEASED");
		break;
		case HTTP_CALLBACK_ALREADY_REGISTERED:
			strcpy(error_message, "HTTP_CALLBACK_ALREADY_REGISTERED");
		break;
		case HTTP_CALLBACK_NOT_REGISTERED:
			strcpy(error_message, "HTTP_CALLBACK_NOT_REGISTERED");
		break;
		case HTTP_NOT_ENOUGH_STORAGE:
			strcpy(error_message, "HTTP_NOT_ENOUGH_STORAGE");
		break;
		case HTTP_INVALID_DICT_ARGS:
			strcpy(error_message, "HTTP_INVALID_DICT_ARGS");
		break;
		case HTTP_INTERNAL_INCONSISTENCY:
			strcpy(error_message, "HTTP_INTERNAL_INCONSISTENCY");
		break;
		case HTTP_INVALID_BRIDGE_RESPONSE:
			strcpy(error_message, "HTTP_INVALID_BRIDGE_RESPONSE");
		break;
		default: {
			strcpy(error_message, "HTTP_ERROR_UNKNOWN");
		}
	}

	text_layer_set_text(&layer_text4, error_message);
}
