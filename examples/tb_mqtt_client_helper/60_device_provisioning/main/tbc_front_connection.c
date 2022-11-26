// Copyright 2022 liangzhuzhi2020@gmail.com, https://github.com/liang-zhu-zi/esp32-thingsboard-mqtt-client
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

#include <stdio.h>
#include <string.h>
//#include <stdarg.h>

//#include "freertos/FreeRTOS.h"
//#include "freertos/queue.h"
//#include "freertos/semphr.h"
#include "sys/queue.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"

#include "tbc_utils.h"

#include "tbc_mqtt.h"
#include "tbc_mqtt_helper.h"

#include "tbc_transport_credentials_memory.h"

//#include "tbc_utils.h"
//#include "timeseriesdata.h"
//#include "clientattribute.h"
//#include "sharedattribute.h"
//#include "attributes_request.h"
//#include "server_rpc.h"
//#include "client_rpc.h"
//#include "device_provision.h"
//#include "ota_update.h"

//#include "device_provision.h"

/**
 * Reference tbc_provison_config_t
 *
| Provisioning request  | Parameter              | Description                                                                    | Credentials generated by <br/>the ThingsBoard server | Devices supplies<br/>Access Token | Devices supplies<br/>Basic MQTT Credentials | Devices supplies<br/>X.509 Certificate |
|-----------------------|------------------------|--------------------------------------------------------------------------------|------------------------------------------------------|-----------------------------------|---------------------------------------------|----------------------------------------|
|                       | deviceName             | Device name in ThingsBoard.                                                    | (O) DEVICE_NAME                                      | (O) DEVICE_NAME                   | (O) DEVICE_NAME                             | (O) DEVICE_NAME                        |
|                       | provisionDeviceKey     | Provisioning device key, you should take it from configured device profile.    | (M) PUT_PROVISION_KEY_HERE                           | (M) PUT_PROVISION_KEY_HERE        | (M) PUT_PROVISION_KEY_HERE                  | (M) PUT_PROVISION_KEY_HERE             |
|                       | provisionDeviceSecret  | Provisioning device secret, you should take it from configured device profile. | (M) PUT_PROVISION_SECRET_HERE                        | (M) PUT_PROVISION_SECRET_HERE     | (M) PUT_PROVISION_SECRET_HERE               | (M) PUT_PROVISION_SECRET_HERE          |
|                       | credentialsType        | Credentials type parameter.                                                    |                                                      | (M) ACCESS_TOKEN                  | (M) MQTT_BASIC                              | (M) X509_CERTIFICATE                   |
|                       | token                  | Access token for device in ThingsBoard.                                        |                                                      | (M) DEVICE_ACCESS_TOKEN           |                                             |                                        |
|                       | clientId               | Client id for device in ThingsBoard.                                           |                                                      |                                   | (M) DEVICE_CLIENT_ID_HERE                   |                                        |
|                       | username               | Username for device in ThingsBoard.                                            |                                                      |                                   | (M) DEVICE_USERNAME_HERE                    |                                        |
|                       | password               | Password for device in ThingsBoard.                                            |                                                      |                                   | (M) DEVICE_PASSWORD_HERE                    |                                        |
|                       | hash                   | Public key X509 hash for device in ThingsBoard.                                |                                                      |                                   |                                             | (M) MIIB……..AQAB                       |
|                       | (O) Optional, (M) Must |
*/
typedef struct tbc_provison_storage
{
  tbc_provison_type_t provisionType; // Generates/Supplies credentials type. // Hardcode or device profile.
 
  char *deviceName;           // Device name in TB        // Chip name + Chip id, e.g., "esp32-C8:D6:93:12:BC:01". Each device is different.
  char *provisionDeviceKey;   // Provision device key     // Hardcode or device profile. Each model is different. 
  char *provisionDeviceSecret;// Provision device secret  // Hardcode or device profile. Each model is different.

  char *token;     // Access token for device             // Randomly generated. Each device is different.
  char *clientId;  // Client id for device                // Randomly generated. Each device is different.
  char *username;  // Username for device                 // Randomly generated. Each device is different.
  char *password;  // Password for device                 // Randomly generated. Each device is different.
  char *hash;      // Public key X509 hash for device     // Public key X509.    Each device is different.
} tbc_provison_storage_t;

static const char *TAG = "FRONT-CONN";
static tbc_provison_storage_t _provision_storage = {0};

static void _provision_storage_free_fields(tbc_provison_storage_t *storage)
{

    TBC_CHECK_PTR(storage);

    storage->provisionType = TBC_PROVISION_TYPE_NONE;
    TBC_FIELD_FREE(storage->deviceName);
    TBC_FIELD_FREE(storage->provisionDeviceKey);
    TBC_FIELD_FREE(storage->provisionDeviceSecret);

    TBC_FIELD_FREE(storage->token);
    TBC_FIELD_FREE(storage->clientId);
    TBC_FIELD_FREE(storage->username);
    TBC_FIELD_FREE(storage->password);
    TBC_FIELD_FREE(storage->hash);
}

static void *_provision_storage_fill_from_config(tbc_provison_storage_t *storage, const tbc_provison_config_t *config)
{
    TBC_CHECK_PTR_WITH_RETURN_VALUE(storage, NULL);
    TBC_CHECK_PTR_WITH_RETURN_VALUE(config, NULL);

    _provision_storage_free_fields(storage);

    storage->provisionType = config->provisionType;
    TBC_FIELD_STRDUP(storage->deviceName, config->deviceName);
    TBC_FIELD_STRDUP(storage->provisionDeviceKey, config->provisionDeviceKey);
    TBC_FIELD_STRDUP(storage->provisionDeviceSecret, config->provisionDeviceSecret);
    
    TBC_FIELD_STRDUP(storage->token, config->token);
    TBC_FIELD_STRDUP(storage->clientId, config->clientId);
    TBC_FIELD_STRDUP(storage->username, config->username);
    TBC_FIELD_STRDUP(storage->password, config->password);
    TBC_FIELD_STRDUP(storage->hash, config->hash);
    return storage;
}

static void *_provision_storage_copy_to_config(const tbc_provison_storage_t *storage, tbc_provison_config_t *config)
{
    TBC_CHECK_PTR_WITH_RETURN_VALUE(storage, NULL);
    TBC_CHECK_PTR_WITH_RETURN_VALUE(config, NULL);

    config->provisionType = storage->provisionType;

    config->deviceName = storage->deviceName;
    config->provisionDeviceKey = storage->provisionDeviceKey;
    config->provisionDeviceSecret = storage->provisionDeviceSecret;

    config->token = storage->token;
    config->clientId = storage->clientId;
    config->username = storage->username;
    config->password = storage->password;
    config->hash = storage->hash;
    return config;
}

static void _tb_provision_on_response(tbcmh_handle_t client, void *context,
                                int request_id, const tbc_transport_credentials_config_t *credentials)
{
   TBC_CHECK_PTR(client);
   TBC_CHECK_PTR(credentials);

   tbc_transport_credentials_memory_save(credentials);

   TBC_LOGE("Provision failurs and the device will not work!");
}

static void _tb_provision_on_timeout(tbcmh_handle_t client, void *context, int request_id)
{
   TBC_LOGE("Provision timeout and the device will not work!");
}

/*!< Callback of connected ThingsBoard MQTT */
void tb_frontconn_on_connected(tbcmh_handle_t client, void *context)
{
    ESP_LOGI(TAG, "FRONT CONN: Connected to thingsboard server!");

    sleep(1);
    tbc_provison_config_t provision_config = {0};
    _provision_storage_copy_to_config(&_provision_storage, &provision_config);
    tbcmh_provision_request(client, &provision_config, NULL,
                            _tb_provision_on_response,
                            _tb_provision_on_timeout);
}

/*!< Callback of disconnected ThingsBoard MQTT */
void tb_frontconn_on_disconnected(tbcmh_handle_t client, void *context)
{
   ESP_LOGI(TAG, "FRONT CONN: Disconnected from thingsboard server!");
}

// self._username = "provision"
// X.509:
// self._username = "provision"
// self.tls_set(ca_certs="mqttserver.pub.pem", tls_version=ssl.PROTOCOL_TLSv1_2)
tbcmh_handle_t tbcmh_frontconn_create(const tbc_transport_config_t *transport,
                                      const tbc_provison_config_t *provision)
{
    if (!transport) {
        ESP_LOGE(TAG, "FRONT CONN: transport is NULL!");
        return NULL;
    }
    if (!provision) {
        ESP_LOGE(TAG, "FRONT CONN: provision is NULL!");
        return NULL;
    }

    ESP_LOGI(TAG, "FRONT CONN: Init tbcmh ...");
    bool is_running_in_mqtt_task = false;
    tbcmh_handle_t client = tbcmh_init(is_running_in_mqtt_task);
    if (!client) {
        ESP_LOGE(TAG, "FRONT CONN: Failure to init tbcmh!");
        return NULL;
    }

    ESP_LOGI(TAG, "FRONT CONN: Connect tbcmh ...");
    // prepare credentials for provision
    tbc_transport_config_t temp = {0};
    tbc_transport_config_copy(&temp, transport);
    temp.credentials.type       = TBC_TRANSPORT_CREDENTIALS_TYPE_NONE;  /*!< ThingsBoard Client transport authentication/credentials type */
    temp.credentials.client_id  = NULL;                                 /*!< MQTT.           default client id is ``ESP32_%CHIPID%`` where %CHIPID% are last 3 bytes of MAC address in hex format */
    temp.credentials.username   = TB_MQTT_PARAM_PROVISION_USERNAME;     /*!< MQTT/HTTP.      username */
    temp.credentials.password   = NULL;                                 /*!< MQTT/HTTP.      password */
    temp.credentials.token      = NULL;                                 /*!< MQTT/HTTP/CoAP: username/path param/path param */
    bool result = tbcmh_connect(client, &temp, TBCMH_FUNCTION_DEVICE_PROVISION, NULL,
                                   tb_frontconn_on_connected,
                                   tb_frontconn_on_disconnected);
    if (!result) {
        ESP_LOGE(TAG, "FRONT CONN: failure to connect to tbcmh!");
        ESP_LOGI(TAG, "FRONT CONN: Destroy tbcmh ...");
        tbcmh_destroy(client);
        return NULL;
    }

    _provision_storage_fill_from_config(&_provision_storage, provision);
    return client;
}

