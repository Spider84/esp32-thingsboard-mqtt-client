/* MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_log.h"

#include "tbc_utils.h"

#include "tbc_transport_config.h"

#include "tbc_mqtt_helper.h"
#include "protocol_examples_common.h"

#include "tbc_transport_credentials_memory.h"

static const char *TAG = "DEVICE_PROVISION_MAIN";

extern tbcmh_handle_t tbcmh_frontconn_create(const tbc_transport_config_t *transport,
                                            const tbc_provison_config_t *provision);
extern tbcmh_handle_t tbcmh_normalconn_create(const tbc_transport_config_t *transport);

/*
 * Define psk key and hint as defined in mqtt broker
 * example for mosquitto server, content of psk_file:
 * hint:BAD123
 *
 */
/*
static const uint8_t s_key[] = { 0xBA, 0xD1, 0x23 };

static const psk_hint_key_t psk_hint_key = {
            .key = s_key,
            .key_size = sizeof(s_key),
            .hint = "hint"
        };
*/


// #if CONFIG_BROKER_CERTIFICATE_OVERRIDDEN == 1
// static const uint8_t mqtt_eclipseprojects_io_pem_start[]  = "-----BEGIN CERTIFICATE-----\n" CONFIG_BROKER_CERTIFICATE_OVERRIDE "\n-----END CERTIFICATE-----";
// #else
// extern const uint8_t mqtt_eclipseprojects_io_pem_start[]   asm("_binary_mqtt_eclipseprojects_io_pem_start");
// #endif
// extern const uint8_t mqtt_eclipseprojects_io_pem_end[]   asm("_binary_mqtt_eclipseprojects_io_pem_end");

extern const uint8_t server_cert_pem_start[] asm("_binary_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_cert_pem_end");

extern const uint8_t client_cert_pem_start[] asm("_binary_client_crt_start");
extern const uint8_t client_cert_pem_end[] asm("_binary_client_crt_end");

extern const uint8_t client_key_pem_start[] asm("_binary_client_key_start");
extern const uint8_t client_key_pem_end[] asm("_binary_client_key_end");

///////// address //////////////////////////////////////////////////////////////

//###### MQTT ##################################################
#if CONFIG_TBC_TRANSPORT_ADDRESS_SHEMA_USE_MQTT
    #define _ADDRESS_SCHEMA  "mqtt"
#elif CONFIG_TBC_TRANSPORT_ADDRESS_SHEMA_USE_MQTTS
    #define _ADDRESS_SCHEMA  "mqtts"
#elif TBC_TRANSPORT_ADDRESS_SHEMA_USE_MQTT_WS   # TB NOT implemented
    #define _ADDRESS_SCHEMA  "ws"
#elif TBC_TRANSPORT_ADDRESS_SHEMA_USE_MQTT_WSS  # TB NOT implemented
    #define _ADDRESS_SCHEMA  "wss"
//###### HTTP ##################################################
#elif TBC_TRANSPORT_ADDRESS_SHEMA_USE_HTTP
    #define _ADDRESS_SCHEMA  "http"
#elif TBC_TRANSPORT_ADDRESS_SHEMA_USE_HTTPS
    #define _ADDRESS_SCHEMA  "https"
//###### CoAP ##################################################
#elif TBC_TRANSPORT_ADDRESS_SHEMA_USE_COAP
    #define _ADDRESS_SCHEMA  "coap"
#elif TBC_TRANSPORT_ADDRESS_SHEMA_USE_COAPS
    #define _ADDRESS_SCHEMA  "coaps"
#else
    #define _ADDRESS_SCHEMA  NULL
#endif

#ifdef CONFIG_TBC_TRANSPORT_ADDRESS_PATH
    #define _ADDRESS_PATH  CONFIG_TBC_TRANSPORT_ADDRESS_PATH
#else
    #define _ADDRESS_PATH  NULL
#endif


///////// provision //////////////////////////////////////////////////////////////

#if CONFIG_TBC_PROVISION_TYPE_SERVER_GENERATES_CREDENTIALS
    #define _PROVISION_TYPE  TBC_PROVISION_TYPE_SERVER_GENERATES_CREDENTIALS          // Credentials generated by the ThingsBoard server
#elif CONFIG_TBC_PROVISION_TYPE_DEVICE_SUPPLIES_ACCESS_TOKEN
    #define _PROVISION_TYPE  TBC_PROVISION_TYPE_DEVICE_SUPPLIES_ACCESS_TOKEN          // Devices supplies Access Token
#elif CONFIG_TBC_PROVISION_TYPE_DEVICE_SUPPLIES_BASIC_MQTT_CREDENTIALS
    #define _PROVISION_TYPE  TBC_PROVISION_TYPE_DEVICE_SUPPLIES_BASIC_MQTT_CREDENTIALS// Devices supplies Basic MQTT Credentials
#elif CONFIG_TBC_PROVISION_TYPE_DEVICE_SUPPLIES_X509_CREDENTIALS
    #define _PROVISION_TYPE  TBC_PROVISION_TYPE_DEVICE_SUPPLIES_X509_CREDENTIALS      // Devices supplies X.509 Certificate
#else
    #define _PROVISION_TYPE  TBC_PROVISION_TYPE_NONE
#endif

#ifdef CONFIG_TBC_PROVISION_TOKEN
    #define _PROVISION_TOKEN  CONFIG_TBC_PROVISION_TOKEN
#else
    #define _PROVISION_TOKEN  NULL
#endif

#ifdef CONFIG_TBC_PROVISION_CLIENT_ID
    #define _PROVISION_CLIENT_ID  CONFIG_TBC_PROVISION_CLIENT_ID
#else
    #define _PROVISION_CLIENT_ID  NULL
#endif

#ifdef CONFIG_TBC_PROVISION_USER_NAME
    #define _PROVISION_USER_NAME  CONFIG_TBC_PROVISION_USER_NAME
#else
    #define _PROVISION_USER_NAME  NULL
#endif

#ifdef CONFIG_TBC_PROVISION_PASSWORD
    #define _PROVISION_PASSWORD  CONFIG_TBC_PROVISION_PASSWORD
#else
    #define _PROVISION_PASSWORD  NULL
#endif

#ifdef CONFIG_TBC_PROVISION_USE_HASH 
    #define _PROVISION_HASH  (const char*)client_cert_pem_start  // TODO: // "Hash / Public key X.509 of device"
#else
    #define _PROVISION_HASH  NULL
#endif


///////// credentials //////////////////////////////////////////////////////////////

#if CONFIG_TBC_TRANSPORT_CREDENTIALS_TYPE_USE_ACCESS_TOKEN
    #define _CREDENTIALS_TYPE  TBC_TRANSPORT_CREDENTIALS_TYPE_ACCESS_TOKEN      /*!< Access Token.          for MQTT, HTTP, CoAP */
#elif CONFIG_TBC_TRANSPORT_CREDENTIALS_TYPE_USE_BASIC_MQTT
    #define _CREDENTIALS_TYPE  TBC_TRANSPORT_CREDENTIALS_TYPE_BASIC_MQTT        /*!< Basic MQTT Credentials.for MQTT             */
#elif CONFIG_TBC_TRANSPORT_CREDENTIALS_TYPE_USE_X509
    #define _CREDENTIALS_TYPE  TBC_TRANSPORT_CREDENTIALS_TYPE_X509              /*!< X.509 Certificate.     for MQTT,       CoAP */
#else
    #define _CREDENTIALS_TYPE  TBC_TRANSPORT_CREDENTIALS_TYPE_NONE              /*!< None. for provision.   for MQTT, HTTP, CoAP */
#endif

#ifdef CONFIG_TBC_TRANSPORT_CREDENTIALS_CLIENT_ID
    #define _CREDENTIALS_CLIENT_ID  CONFIG_TBC_TRANSPORT_CREDENTIALS_CLIENT_ID
#else
    #define _CREDENTIALS_CLIENT_ID  NULL
#endif

#ifdef CONFIG_TBC_TRANSPORT_CREDENTIALS_USERNAME
    #define _CREDENTIALS_USERNAME   CONFIG_TBC_TRANSPORT_CREDENTIALS_USERNAME
#else
    #define _CREDENTIALS_USERNAME   NULL
#endif

#ifdef CONFIG_TBC_TRANSPORT_CREDENTIALS_PASSWORD
    #define _CREDENTIALS_PASSWORD   CONFIG_TBC_TRANSPORT_CREDENTIALS_PASSWORD
#else
    #define _CREDENTIALS_PASSWORD   NULL
#endif

#ifdef CONFIG_TBC_TRANSPORT_CREDENTIALS_TOKEN
    #define _CREDENTIALS_TOKEN    CONFIG_TBC_TRANSPORT_CREDENTIALS_TOKEN
#else
    #define _CREDENTIALS_TOKEN    NULL
#endif


///////// verification //////////////////////////////////////////////////////////////

#if CONFIG_TBC_TRANSPORT_USE_CERT_PEM //"Use certificate data in PEM format for server verify (with SSL)"
    #define _VERIFICATION_CERT_PEM  (const char *)server_cert_pem_start
#else
    #define _VERIFICATION_CERT_PEM  NULL
#endif


#if CONFIG_TBC_TRANSPORT_SKIP_CERT_COMMON_NAME_CHECK //"Skip any validation of server certificate CN field"
    #define _VERIFICATION_SKIP_CERT_COMMON_NAME_CHECK   true
#else
    #define _VERIFICATION_SKIP_CERT_COMMON_NAME_CHECK   false
#endif


///////// authentication //////////////////////////////////////////////////////////////

#if CONFIG_TBC_TRANSPORT_USE_CLIENT_CERT_PEM    //"Use certificate data in PEM format for for SSL mutual authentication"
    #define _AUTHENTICATION_CLIENT_CERT_PEM  (const char *)client_cert_pem_start
#else
    #define _AUTHENTICATION_CLIENT_CERT_PEM  NULL
#endif

#if CONFIG_TBC_TRANSPORT_USE_CLIENT_KEY_PEM     //"Use private key data in PEM format for SSL mutual authentication"
    #define _AUTHENTICATION_CLIENT_KEY_PEM   (const char *)client_key_pem_start
#else
    #define _AUTHENTICATION_CLIENT_KEY_PEM   NULL
#endif


static tbc_transport_address_config_t _address = /*!< MQTT: broker, HTTP: server, CoAP: server */
            {
                //bool tlsEnabled,                              /*!< Enabled TLS/SSL or DTLS */
                .schema = _ADDRESS_SCHEMA,                      /*!< MQTT: mqtt/mqtts/ws/wss, HTTP: http/https, CoAP: coap/coaps */
                .host   = CONFIG_TBC_TRANSPORT_ADDRESS_HOST,    /*!< MQTT/HTTP/CoAP server domain, hostname, to set ipv4 pass it as string */
                .port   = CONFIG_TBC_TRANSPORT_ADDRESS_PORT,    /*!< MQTT/HTTP/CoAP server port */
                .path   = _ADDRESS_PATH,                        /*!< Path in the URI*/
            };

#if CONFIG_TBC_TRANSPORT_WITH_PROVISION
static tbc_provison_config_t  _provision = 
            {
              .provisionType = _PROVISION_TYPE,                 // Generates/Supplies credentials type. // Hardcode or device profile.
             
              .deviceName = CONFIG_TBC_PROVISION_DEVICE_NAME,               // Device name in TB        // Chip name + Chip id, e.g., "esp32-C8:D6:93:12:BC:01". Each device is different.
              .provisionDeviceKey = CONFIG_TBC_PROVISION_DEVICE_KEY,        // Provision device key     // Hardcode or device profile. Each model is different. 
              .provisionDeviceSecret = CONFIG_TBC_PROVISION_DEVICE_SECRET,  // Provision device secret  // Hardcode or device profile. Each model is different.

              .token    = _PROVISION_TOKEN,     // Access token for device             // Randomly generated. Each device is different.
              .clientId = _PROVISION_CLIENT_ID, // Client id for device                // Randomly generated. Each device is different.
              .username = _PROVISION_USER_NAME, // Username for device                 // Randomly generated. Each device is different.
              .password = _PROVISION_PASSWORD,  // Password for device                 // Randomly generated. Each device is different.
              .hash     = _PROVISION_HASH,      // Public key X509 hash for device     // Public key X509.    Each device is different.
            };
#else
static tbc_transport_credentials_config_t _credentials = /*!< Client related credentials for authentication */
            {       
                .type = _CREDENTIALS_TYPE,              /*!< ThingsBoard Client transport authentication/credentials type */
                
                .client_id = _CREDENTIALS_CLIENT_ID,    /*!< MQTT.           default client id is ``ESP32_%CHIPID%`` where %CHIPID% are last 3 bytes of MAC address in hex format */
                .username = _CREDENTIALS_USERNAME,      /*!< MQTT/HTTP.      username */
                .password = _CREDENTIALS_PASSWORD,      /*!< MQTT/HTTP.      password */
                
                .token = _CREDENTIALS_TOKEN,            /*!< MQTT/HTTP/CoAP: username/path param/path param */
                                                        /*!< At TBC_TRANSPORT_CREDENTIALS_TYPE_X509 it's a client public key. DON'T USE IT! */
            };
#endif

static tbc_transport_verification_config_t _verification = /*!< Security verification of the broker/server */
            {       
                 //bool      use_global_ca_store;               /*!< Use a global ca_store, look esp-tls documentation for details. */
                 //esp_err_t (*crt_bundle_attach)(void *conf); 
                                                                /*!< Pointer to ESP x509 Certificate Bundle attach function for the usage of certificate bundles. */
                 .cert_pem = _VERIFICATION_CERT_PEM,            /*!< Pointer to certificate data in PEM or DER format for server verify (with SSL), default is NULL, not required to verify the server. PEM-format must have a terminating NULL-character. DER-format requires the length to be passed in cert_len. */
                 .cert_len = 0,                                 /*!< Length of the buffer pointed to by cert_pem. May be 0 for null-terminated pem */
                 //.psk_hint_key = &psk_hint_key,
                                                                /*!< Pointer to PSK struct defined in esp_tls.h to enable PSK
                                                                  authentication (as alternative to certificate verification).
                                                                  PSK is enabled only if there are no other ways to
                                                                  verify broker.*/
                 .skip_cert_common_name_check = _VERIFICATION_SKIP_CERT_COMMON_NAME_CHECK,
                                                                /*!< Skip any validation of server certificate CN field, this reduces the security of TLS and makes the mqtt client susceptible to MITM attacks  */
                 //const char **alpn_protos;                    /*!< NULL-terminated list of supported application protocols to be used for ALPN */
            };

static tbc_transport_authentication_config_t _authentication = /*!< Client authentication for mutual authentication using TLS */
            {
                .client_cert_pem = _AUTHENTICATION_CLIENT_CERT_PEM, /*!< Pointer to certificate data in PEM or DER format for SSL mutual authentication, default is NULL, not required if mutual authentication is not needed. If it is not NULL, also `client_key_pem` has to be provided. PEM-format must have a terminating NULL-character. DER-format requires the length to be passed in client_cert_len. */
                .client_cert_len = 0,                               /*!< Length of the buffer pointed to by client_cert_pem. May be 0 for null-terminated pem */
                .client_key_pem = _AUTHENTICATION_CLIENT_KEY_PEM,   /*!< Pointer to private key data in PEM or DER format for SSL mutual authentication, default is NULL, not required if mutual authentication is not needed. If it is not NULL, also `client_cert_pem` has to be provided. PEM-format must have a terminating NULL-character. DER-format requires the length to be passed in client_key_len */
                .client_key_len = 0,                                /*!< Length of the buffer pointed to by client_key_pem. May be 0 for null-terminated pem */
                .client_key_password = NULL,                        /*!< Client key decryption password string */
                .client_key_password_len = 0,                       /*!< String length of the password pointed to by client_key_password */
                //bool      use_secure_element;                     /*!< Enable secure element, available in ESP32-ROOM-32SE, for SSL connection */
                //void     *ds_data;                                /*!< Carrier of handle for digital signature parameters, digital signature peripheral is available in some Espressif devices. */
            };


#if CONFIG_TBC_TRANSPORT_WITH_PROVISION
static void mqtt_app_start()
{
    tbcmh_handle_t client = NULL;
    bool is_front_connection = false;

    tbc_transport_credentials_memory_init();
    tbc_transport_credentials_memory_clean();

    //init transport
    tbc_transport_config_t transport = {0};
    memcpy(&transport.address, &_address, sizeof(_address));
    //memcpy(&transport.credentials, &_credentials, sizeof(_credentials));
    memcpy(&transport.verification, &_verification, sizeof(_verification));
    //memcpy(&transport.authentication, &_authentication, sizeof(_authentication));
    transport.log_rxtx_package = true;  /*!< print Rx/Tx MQTT package */

    // create_front_connect(client)
    client = tbcmh_frontconn_create(&transport, &_provision);
    is_front_connection = true;

    int i = 0;
    while (client && i<40) // TB_MQTT_TIMEOUT is 30 seconds!
    {
        if (tbcmh_has_events(client)) {
            tbcmh_run(client);
        }

        if (is_front_connection==true && tbc_transport_credentials_memory_is_existed()) {
            // destory_front_conn(client)
            if (client) tbcmh_disconnect(client);
            if (client) tbcmh_destroy(client);
            client = NULL;

            // create_normal_connect(client) // If the credentials type is X.509, the front connection (provisioing) is one-way SSL, but the normal connection is two-way (mutual) SSL.
            memcpy(&transport.address, &_address, sizeof(_address));
            _transport_credentials_config_copy(&transport.credentials, tbc_transport_credentials_memory_get()); //memcpy(&transport.credentials, &_credentials, sizeof(_credentials));
            memcpy(&transport.verification, &_verification, sizeof(_verification));
            memcpy(&transport.authentication, &_authentication, sizeof(_authentication));
            client = tbcmh_normalconn_create(&transport);
            is_front_connection = false;
        }

        i++;
        if (tbcmh_is_connected(client)) {
            //no code
        } else {
            ESP_LOGI(TAG, "Still NOT connected to server!");
        }
        sleep(1);
    };

    ESP_LOGI(TAG, "Disconnect tbcmh ...");
    if (client) {
        tbcmh_disconnect(client);
    }
    ESP_LOGI(TAG, "Destroy tbcmh ...");
    if (client) {
        tbcmh_destroy(client);
        client = NULL;
    }

    tbc_transport_credentials_memory_uninit();
}

#else // !CONFIG_TBC_TRANSPORT_WITH_PROVISION

static void mqtt_app_start()
{
    tbcmh_handle_t client = NULL;
    //bool is_front_connection = false;

    //tbc_transport_credentials_memory_init();
    //tbc_transport_credentials_memory_clean();

    //init transport
    tbc_transport_config_t transport = {0};
    memcpy(&transport.address, &_address, sizeof(_address));
    memcpy(&transport.credentials, &_credentials, sizeof(_credentials));
    memcpy(&transport.verification, &_verification, sizeof(_verification));
    memcpy(&transport.authentication, &_authentication, sizeof(_authentication));
    transport.log_rxtx_package = true;  /*!< print Rx/Tx MQTT package */

    // create_normal_connect(client)
    client = tbcmh_normalconn_create(&transport);
    //is_front_connection = false;

    int i = 0;
    while (client && i<40) // TB_MQTT_TIMEOUT is 30 seconds!
    {
        if (tbcmh_has_events(client)) {
            tbcmh_run(client);
        }

        i++;
        if (tbcmh_is_connected(client)) {
            //no code
        } else {
            ESP_LOGI(TAG, "Still NOT connected to server!");
        }
        sleep(1);
    };

    ESP_LOGI(TAG, "Disconnect tbcmh ...");
    if (client) {
        tbcmh_disconnect(client);
    }
    ESP_LOGI(TAG, "Destroy tbcmh ...");
    if (client) {
        tbcmh_destroy(client);
        client = NULL;
    }

    //tbc_transport_credentials_memory_uninit();
}
#endif

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO); //ESP_LOG_DEBUG
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    //esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);
    esp_log_level_set("tb_mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("tb_mqtt_client_helper", ESP_LOG_VERBOSE);
    esp_log_level_set("attributes_reques", ESP_LOG_VERBOSE);
    esp_log_level_set("clientattribute", ESP_LOG_VERBOSE);
    esp_log_level_set("client_rpc", ESP_LOG_VERBOSE);
    esp_log_level_set("ota_update", ESP_LOG_VERBOSE);
    esp_log_level_set("server_rpc", ESP_LOG_VERBOSE);
    esp_log_level_set("sharedattribute", ESP_LOG_VERBOSE);
    esp_log_level_set("timeseriesdata", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    mqtt_app_start();
}

#if 0
static char *get_string_from_stdin(const char* descriptiton, char* buffer, size_t size)
{
    TBC_CHECK_PTR_WITH_RETURN_VALUE(buffer, NULL);
    if (size==0) {
        ESP_LOGE(TAG, "size is ZERO! %s() %d", __FUNCTION__, __LINE__);
        return NULL;
    }

    int count = 0;
    printf("Please enter %s\n", descriptiton);
    while (count < size) {
        int c = fgetc(stdin);
        if (c == '\n') {
            buffer[count] = '\0';
            break;
        } else if (c > 0 && c < size-1) {
            buffer[count] = c;
            ++count;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    buffer[count] = '\0';

    printf("%s: %s\n", descriptiton, buffer);
    return buffer;
}

static void mqtt_config_get(void)
{
	//tbc_err_t err;
#if 0
    const esp_mqtt_client_config_t config = {
        .uri = CONFIG_BROKER_URL
    };
#else
    const char *access_token = CONFIG_ACCESS_TOKEN;
    const char *uri = CONFIG_BROKER_URL;
#endif

#if CONFIG_BROKER_URL_FROM_STDIN
    char line_uri[128];

    if (strcmp(uri, "FROM_STDIN") == 0) {
        get_string_from_stdin("broker url", line_uri, sizeof(line_uri));
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif /* CONFIG_BROKER_URL_FROM_STDIN */

#if CONFIG_ACCESS_TOKEN_FROM_STDIN
    char line_token[128];

    if (strcmp(access_token, "FROM_STDIN") == 0) { //mqtt_cfg.
        get_string_from_stdin("access token", line_token, sizeof(line_token));
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong access token");
        abort();
    }
#endif /* CONFIG_ACCESS_TOKEN_FROM_STDIN */
}
#endif

