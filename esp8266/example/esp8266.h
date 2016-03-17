// Combined include file for esp8266

// This file is from esphttpd, beerware license.

// (customized)

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <c_types.h>
#include <ip_addr.h>
#include <espconn.h>
#include <ets_sys.h>
#include <gpio.h>
#include <mem.h>
#include <osapi.h>
#include <user_interface.h>
#include <upgrade.h>

// because I'm lazy and it's too long...
#define FLASH_FN ICACHE_FLASH_ATTR

#include "platform.h"
#include "espmissingincludes.h"
#include "esp_sdk_ver.h"
