#ifndef __ynet_H__
#define __ynet_H__

#include <app.h>
#include <Elementary.h>
#include <system_settings.h>
#include <efl_extension.h>
#include <dlog.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <curl/curl.h>
#include <net_connection.h>

#include <http.h>
#include "cJSON_Utils.h"
#include "cJSON.h"

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "ynet"

#if !defined(PACKAGE)
#define PACKAGE "il.co.ynet"
#define articles_url "https://www.ynet.co.il/iphone/json/0,,8,00.js"
#define flash_url "https://www.ynet.co.il/Iphone/0,13257,SubCategory-V9-184,00.js"
#define weather_url "https://ynetmobileservices.yit.co.il/weather/ynet/ipad"
#endif

#endif /* __ynet_H__ */
