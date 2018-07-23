#include "ynet.h"
#define articles_url "https://www.ynet.co.il/iphone/json/0,,8,00.js"
#define flash_url "https://www.ynet.co.il/Iphone/0,13257,SubCategory-V9-184,00.js"
#define weather_url "https://ynetmobileservices.yit.co.il/weather/ynet/ipad"

typedef struct appdata
{
	Evas_Object *window;
	Evas_Object *conformant;
	Evas_Object *main_naviframe;
	Evas_Object *articles_naviframe;
	Evas_Object *flash_naviframe;
	Evas_Object *weather_naviframe;
	Evas_Object *gesture_layer;
	Evas_Object *main_articles_background;
	Evas_Object *main_flash_background;
	Evas_Object *main_weather_background;
	Elm_Object_Item *main_articles_navi_item;
	Elm_Object_Item *main_flash_navi_item;
	Elm_Object_Item *main_weather_navi_item;
	Elm_Object_Item *articles_navi_item;
	Elm_Object_Item *flash_navi_item;
	Elm_Object_Item *weather_navi_item;
	cJSON *articles_array;
	cJSON *flash_array;
	cJSON *weather_array;
} appdata_s;

struct MemoryStruct
{
	char *memory;
	size_t size;
};

static void win_delete_request_cb(void *data, Evas_Object *obj, void *event_info)
{
	ui_app_exit();
}

static void app_get_full_resource_path(char *file_path);

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *) userp;

	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL)
	{
		/* out of memory! */
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = "\0";

	return realsize;
}

int download_json(char *url, void *data)
{
	CURL *curl_handle;
	CURLcode res;

	struct MemoryStruct chunk;

	appdata_s *app_data = data;

	chunk.memory = malloc(1); /* will be grown as needed by the realloc above */
	chunk.size = 0; /* no data at this point */

	curl_global_init(CURL_GLOBAL_ALL);

	/* init the curl session */
	curl_handle = curl_easy_init();

	/* specify URL to get */
	curl_easy_setopt(curl_handle, CURLOPT_URL, url);

	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0);

	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0);

	/* send all data to this function  */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void * )&chunk);

	/* some servers don't like requests that are made without a user-agent
	 field, so we provide one */
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	/* get it! */
	res = curl_easy_perform(curl_handle);

	/* check for errors */
	if (res != CURLE_OK)
	{
		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
	} else
	{
		/*
		 * Now, our chunk.memory points to a memory block that is chunk.size
		 * bytes big and contains the remote file.
		 *
		 * Do something nice with it!
		 */

		printf("%lu bytes retrieved\n", (unsigned long) chunk.size);
	}

	/* cleanup curl stuff */
	curl_easy_cleanup(curl_handle);

	cJSON *root = cJSON_Parse(chunk.memory);

	if(url == articles_url){
		app_data->articles_array = cJSON_GetObjectItem(root, "components");
	}
	else if(url == flash_url){
		app_data->flash_array = cJSON_GetObjectItem(root->child->child, "item");
	}
	else if(url == weather_url){
		app_data->weather_array = cJSON_GetObjectItem(root->child, "items");
	}

	free(chunk.memory);

	/* we're done with libcurl, so clean it up */
	curl_global_cleanup();
	return 0;
}

/**
 * Act on touch event.
 *
 * If on main navigation frame - push relevant frame.
 *
 * @param[in] data data sent to function
 * @param[in] event_info information about the event
 *
 */
static Evas_Event_Flags touch_event_callback(void *data, void *event_info)
{
	appdata_s *app_data = data;
	Elm_Object_Item *top = elm_naviframe_top_item_get(app_data->main_naviframe);
	dlog_print(DLOG_DEBUG, LOG_TAG, "touch event initiated");
	if (top == app_data->main_articles_navi_item)
	{
		app_data->articles_navi_item = elm_naviframe_item_push(app_data->main_naviframe, NULL, NULL, NULL, app_data->articles_naviframe,
		NULL);
		elm_naviframe_item_title_enabled_set(app_data->articles_navi_item,
		EINA_FALSE, EINA_FALSE);
	} else if (top == app_data->main_flash_navi_item)
	{
		app_data->flash_navi_item = elm_naviframe_item_push(app_data->main_naviframe, NULL, NULL, NULL, app_data->flash_naviframe,
		NULL);
		elm_naviframe_item_title_enabled_set(app_data->flash_navi_item,
		EINA_FALSE, EINA_FALSE);
	} else if (top == app_data->main_weather_navi_item)
	{
		app_data->weather_navi_item = elm_naviframe_item_push(app_data->main_naviframe, NULL, NULL, NULL, app_data->weather_naviframe,
		NULL);
		elm_naviframe_item_title_enabled_set(app_data->weather_navi_item,
		EINA_FALSE, EINA_FALSE);
	}

	return EVAS_EVENT_FLAG_ON_HOLD;
}

/**
 * Act on a back button pressed event.
 *
 * If on main navigation frame - minimize app.
 * Else - pop one frame
 *
 * @param[in] data data sent to function
 * @param[in] event_info information about the event
 *
 */
static void back_event_callback(void *data, void *event_info)
{
	appdata_s *app_data = data;
	Elm_Object_Item *top = elm_naviframe_top_item_get(app_data->main_naviframe);
	dlog_print(DLOG_DEBUG, LOG_TAG, "Back button pressed");
// If top frame is one of the main screens - minimize app
	if (top == app_data->main_articles_navi_item || top == app_data->main_flash_navi_item || top == app_data->main_weather_navi_item)
	{
		/* Let window go to hide state. */
		elm_win_lower(app_data->window);
	}
// Else pop a frame from main navigation frame
	else if (top == app_data->articles_navi_item || top == app_data->flash_navi_item || top == app_data->weather_navi_item)
	{
		elm_naviframe_item_pop(app_data->main_naviframe);
	}
}

/**
 * Get the full path to resource file.
 *
 * Modifies partial path into full path.
 * Parameter has to leave enough space for full path.
 *
 * @param[in] file_path path of file after /opt/usr/globalapps/il.co.ynet/res/
 * with enough space to switch with full path.
 */
static void app_get_full_resource_path(char *file_path)
{
	char *res_path = app_get_resource_path();
	char full_path[256] = "";
	strcat(full_path, res_path);
	strcat(full_path, file_path);
	strcpy(file_path, full_path);
}

/**
 * Act on a rotary event.
 *
 * Modifies partial path into full path.
 * Parameter has to leave enough space for full path.
 *
 * @param[in] data data sent to function
 * @param[in] event_info information about the event (rotation direction)
 *
 */
Eina_Bool rotary_event_callback(void *data, Eext_Rotary_Event_Info *event_info)
{
	appdata_s *app_data = data;
	Elm_Object_Item *bottom = elm_naviframe_bottom_item_get(app_data->main_naviframe);
	dlog_print(DLOG_DEBUG, LOG_TAG, "Rotary device rotated");
	elm_naviframe_item_promote(bottom);
	return EINA_FALSE;
}

static void create_base_gui(appdata_s *app_data)
{
	/* Window */
	/* Create and initialize elm_win.
	 elm_win is mandatory to manipulate window. */
	app_data->window = elm_win_util_standard_add(PACKAGE, PACKAGE);
	elm_win_autodel_set(app_data->window, EINA_TRUE);

	if (elm_win_wm_rotation_supported_get(app_data->window))
	{
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(app_data->window, (const int *) (&rots), 4);
	}

	evas_object_smart_callback_add(app_data->window, "delete,request", win_delete_request_cb, NULL);
	eext_object_event_callback_add(app_data->window, EEXT_CALLBACK_BACK, back_event_callback, app_data);

	/* Conformant */
	/* Create and initialize elm_conformant.
	 elm_conformant is mandatory for base gui to have proper size
	 when indicator or virtual keypad is visible. */
	app_data->conformant = elm_conformant_add(app_data->window);
	elm_win_indicator_mode_set(app_data->window, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(app_data->window, ELM_WIN_INDICATOR_OPAQUE);
	evas_object_size_hint_weight_set(app_data->conformant, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(app_data->window, app_data->conformant);
	evas_object_show(app_data->conformant);

	/* Main Navigation Frame */
	/* Create and initialize elm_naviframe.
	 elm_naviframe is mandatory to iterate between different views.
	 navigation frame acts as a stack. */
	app_data->main_naviframe = elm_naviframe_add(app_data->conformant);
	elm_naviframe_content_preserve_on_pop_set(app_data->main_naviframe, EINA_TRUE);
	evas_object_show(app_data->main_naviframe);
	elm_object_content_set(app_data->conformant, app_data->main_naviframe);

	/* Articles Navigation Frame */
	/* Create and initialize elm_naviframe.
	 elm_naviframe is mandatory to iterate between different views.
	 navigation frame acts as a stack. */
	app_data->articles_naviframe = elm_naviframe_add(app_data->main_naviframe);
	elm_naviframe_content_preserve_on_pop_set(app_data->articles_naviframe, EINA_TRUE);
	evas_object_show(app_data->articles_naviframe);

	/* Flash Navigation Frame */
	/* Create and initialize elm_naviframe.
	 elm_naviframe is mandatory to iterate between different views.
	 navigation frame acts as a stack. */
	app_data->flash_naviframe = elm_naviframe_add(app_data->main_naviframe);
	elm_naviframe_content_preserve_on_pop_set(app_data->flash_naviframe, EINA_TRUE);
	evas_object_show(app_data->flash_naviframe);

	/* Weather Navigation Frame */
	/* Create and initialize elm_naviframe.
	 elm_naviframe is mandatory to iterate between different views.
	 navigation frame acts as a stack. */
	app_data->weather_naviframe = elm_naviframe_add(app_data->main_naviframe);
	elm_naviframe_content_preserve_on_pop_set(app_data->weather_naviframe, EINA_TRUE);
	evas_object_show(app_data->weather_naviframe);

	/* Gesture layer object */
	app_data->gesture_layer = elm_gesture_layer_add(app_data->conformant);
	elm_gesture_layer_attach(app_data->gesture_layer, app_data->main_naviframe);
	/* Call back for touch event */
	elm_gesture_layer_cb_set(app_data->gesture_layer, ELM_GESTURE_N_TAPS, ELM_GESTURE_STATE_START, touch_event_callback, app_data);

	/* Articles Navigation Frame */
	/* Create and initialize elm_naviframe.
	 elm_naviframe is mandatory to iterate between different views.
	 navigation frame acts as a stack. */
	app_data->articles_navi_item = elm_naviframe_add(app_data->main_naviframe);
	evas_object_show(app_data->articles_navi_item);

	/* FLash Navigation Frame */
	/* Create and initialize elm_naviframe.
	 elm_naviframe is mandatory to iterate between different views.
	 navigation frame acts as a stack. */
	app_data->flash_navi_item = elm_naviframe_add(app_data->main_naviframe);
	evas_object_show(app_data->flash_navi_item);

	/* FLash Navigation Frame */
	/* Create and initialize elm_naviframe.
	 elm_naviframe is mandatory to iterate between different views.
	 navigation frame acts as a stack. */
	app_data->weather_navi_item = elm_naviframe_add(app_data->main_naviframe);
	evas_object_show(app_data->weather_navi_item);

	/* Background - Articles */
	/* Create and initialize elm_background.
	 elm_background is used to display background.
	 */
	char main_articles_background_file_path[256] = "img/main/main_articles.png";
	app_data->main_articles_background = elm_bg_add(app_data->main_naviframe);
	app_get_full_resource_path(main_articles_background_file_path);
	elm_bg_file_set(app_data->main_articles_background, main_articles_background_file_path, NULL);

	/* Background - Flash */
	/* Create and initialize elm_background.
	 elm_background is used to display background.
	 */
	char main_flash_background_file_path[256] = "img/main/main_flash.png";
	app_data->main_flash_background = elm_bg_add(app_data->main_naviframe);
	app_get_full_resource_path(main_flash_background_file_path);
	elm_bg_file_set(app_data->main_flash_background, main_flash_background_file_path, NULL);

	/* Background - Weather */
	/* Create and initialize elm_background.
	 elm_background is used to display background.
	 */
	char main_weather_background_file_path[256] = "img/main/main_weather.png";
	app_data->main_weather_background = elm_bg_add(app_data->main_naviframe);
	app_get_full_resource_path(main_weather_background_file_path);
	elm_bg_file_set(app_data->main_weather_background, main_weather_background_file_path, NULL);

// Push all main backgrounds to main navigation frame and disable titles
	app_data->main_articles_navi_item = elm_naviframe_item_push(app_data->main_naviframe, NULL, NULL, NULL,
			app_data->main_articles_background,
			NULL);
	elm_naviframe_item_title_enabled_set(app_data->main_articles_navi_item,
	EINA_FALSE, EINA_FALSE);
	app_data->main_weather_navi_item = elm_naviframe_item_push(app_data->main_naviframe, NULL, NULL, NULL,
			app_data->main_weather_background,
			NULL);
	elm_naviframe_item_title_enabled_set(app_data->main_weather_navi_item,
	EINA_FALSE, EINA_FALSE);
	app_data->main_flash_navi_item = elm_naviframe_item_push(app_data->main_naviframe, NULL, NULL, NULL, app_data->main_flash_background,
	NULL);
	elm_naviframe_item_title_enabled_set(app_data->main_flash_navi_item,
	EINA_FALSE, EINA_FALSE);

	/* Show window after base gui is set up */
	evas_object_show(app_data->window);
}

static bool app_create(void *data)
{
	/* Hook to take necessary actions before main event loop starts
	 Initialize UI resources and application's data
	 If this function returns true, the main loop of application starts
	 If this function returns false, the application is terminated */
	appdata_s *app_data = data;

	int article = download_json(articles_url, app_data);
	int flash = download_json(flash_url, app_data);
	int weather = download_json(weather_url, app_data);

	create_base_gui(app_data);

	/* Call back for rotary event */
	eext_rotary_event_handler_add(rotary_event_callback, app_data);

	return true;
}

static void app_control(app_control_h app_control, void *data)
{
	/* Handle the launch request. */
}

static void app_pause(void *data)
{
	/* Take necessary actions when application becomes invisible. */

}

static void app_resume(void *data)
{
	/* Take necessary actions when application becomes visible. */
}

static void app_terminate(void *data)
{
	/* Release all resources. */
}

static void ui_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LANGUAGE_CHANGED*/
	char *locale = NULL;
	system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, &locale);
	elm_language_set(locale);
	free(locale);
	return;
}

static void ui_app_orient_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_DEVICE_ORIENTATION_CHANGED*/
	return;
}

static void ui_app_region_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}

static void ui_app_low_battery(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_BATTERY*/
}

static void ui_app_low_memory(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_MEMORY*/
}

int main(int argc, char *argv[])
{
	appdata_s ad = { 0, };
	int ret = 0;

	ui_app_lifecycle_callback_s event_callback = { 0, };
	app_event_handler_h handlers[5] = { NULL, };

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_control;

	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, ui_app_low_battery, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, ui_app_low_memory, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED], APP_EVENT_DEVICE_ORIENTATION_CHANGED, ui_app_orient_changed,
			&ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, ui_app_lang_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, ui_app_region_changed, &ad);

	ret = ui_app_main(argc, argv, &event_callback, &ad);
	if (ret != APP_ERROR_NONE)
	{
		dlog_print(DLOG_ERROR, LOG_TAG, "app_main() is failed. err = %d", ret);
	}

	return ret;
}
