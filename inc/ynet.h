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

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "ynet"

#if !defined(PACKAGE)
#define PACKAGE "il.co.ynet"
#endif

#endif /* __ynet_H__ */
