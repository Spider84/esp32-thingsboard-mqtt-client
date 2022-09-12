// Copyright 2022 liangzhuzhi2020@gmail.com, https://github.com/liang-zhu-zi/thingsboard-mqttclient-basedon-espmqtt
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// ThingsBoard MQTT Client high layer API

#ifndef _TB_MQTT_CLIENT_HELPER_LOG_H_
#define _TB_MQTT_CLIENT_HELPER_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_log.h"

#define TBMCH_LOGE(format, ...)  ESP_LOGE(TAG, "[TBMCH][E] " format, ##__VA_ARGS__)
#define TBMCH_LOGW(format, ...)  ESP_LOGE(TAG, "[TBMCH][W] " format, ##__VA_ARGS__)
#define TBMCH_LOGI(format, ...)  ESP_LOGE(TAG, "[TBMCH][I] " format, ##__VA_ARGS__)
#define TBMCH_LOGD(format, ...)  ESP_LOGE(TAG, "[TBMCH][D] " format, ##__VA_ARGS__)
#define TBMCH_LOGV(format, ...)  ESP_LOGE(TAG, "[TBMCH][V] " format, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif //__cplusplus

#endif
