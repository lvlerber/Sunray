/*
 * This file is inspired on the ESP32-XBee distribution (https://github.com/nebkat/esp32-xbee).
 * Copyright (c) 2019 Nebojsa Cvetkovic.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "WebServices.h"
#include <algorithm> // std::min
// #include <cJSON.h>
// #include "AsyncJson.h"
#include "ArduinoJson.h"
#include "confnvs.h"
#include <bits/stdc++.h>
#include <SD.h>

#include <base64.h>
#define DEBUG
#define DIR_PUBLIC "/"
#define BUFFER_SIZE 2048
#define CONFIG_VALUE_UNCHANGED "**********"
static const char *TAG = "WEB";
enum auth_method
{
    AUTH_METHOD_OPEN = 0,
    AUTH_METHOD_HOTSPOT = 1,
    AUTH_METHOD_BASIC = 2
};
static const char *basic_authentication;
static enum auth_method auth_method;
char contentTypes[][2][32] = {
  {".html", "text/html"},
  {".css",  "text/css"},
  {".js",   "application/javascript"},
  {".json", "application/json"},
  {".png",  "image/png"},
  {".jpg",  "image/jpg"},
  {"", ""}
};

#define IS_FILE_EXT(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)

// Set HTTP response content type according to file extension
/// this is probably included in the FS version of beginResponse!!!
static String set_content_type_from_file(HTTPRequest *req, const char *filename)
{
    if (IS_FILE_EXT(filename, ".html"))
    {
        return "text/html";
    }
    else if (IS_FILE_EXT(filename, ".js"))
    {
        return "application/javascript";
    }
    else if (IS_FILE_EXT(filename, ".css"))
    {
        return "text/css";
    }
    else if (IS_FILE_EXT(filename, ".ico"))
    {
        return "image/x-icon";
    }
    /* This is a limited set only */
    /* For any other type always set as plain text */
    return "text/plain";
}

/* Copies the full path into destination buffer and returns
 * pointer to path (skipping the preceding base path) */
/* static char *get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize)
{
    const size_t base_pathlen = strlen(base_path);
    size_t pathlen = strlen(uri);

    const char *quest = strchr(uri, '?');
    if (quest)
    {
        pathlen = std::min((int)pathlen, quest - uri);
    }
    const char *hash = strchr(uri, '#');
    if (hash)
    {
        pathlen = std::min((int)pathlen, hash - uri);
    }

    if (base_pathlen + pathlen + 1 > destsize)
    {
        // Full path string won't fit into destination buffer
        return NULL;
    }

    // Construct full path (base + path)
    strcpy(dest, base_path);
    strlcpy(dest + base_pathlen, uri, pathlen + 1);

    // Return pointer to path, skipping the base
    return dest + base_pathlen;
}
 */
static esp_err_t basic_auth(HTTPRequest *request, HTTPResponse *response)
{

    if (request->getHeader("Authorization").compare("") != 0)
    {

        //   Serial.printf("MyHeader: %s\n", h->value().c_str());
        bool authenticated = strcasecmp(basic_authentication, request->getHeader("Authorization").c_str()) == 0;
        if (authenticated)
            return ESP_OK;
    }
    else
    {
        response->setStatusCode(401);
        response->setStatusText("401 Unauthorized - Incorrect or no password provided");
        response->setHeader("Content-Type", "text/html");
        response->setHeader("WWW-Authenticate", "Basic realm=\"ESP32 Ardumower Config\"");

        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t hotspot_auth(HTTPRequest *req)
{
    // int sock = httpd_req_to_sockfd(req);

    // struct sockaddr_in6 client_addr;
    // socklen_t socklen = sizeof(client_addr);
    // getpeername(sock, (struct sockaddr *)&client_addr, &socklen);

    // // TODO: Correctly read IPv4?
    // // ERROR_ACTION(TAG, client_addr.sin6_family != AF_INET, goto _auth_error, "IPv6 connections not supported, IP family %d", client_addr.sin6_family);

    // wifi_sta_list_t *ap_sta_list = wifi_ap_sta_list();
    // esp_netif_sta_list_t esp_netif_ap_sta_list;
    // esp_netif_get_sta_list(ap_sta_list, &esp_netif_ap_sta_list);

    //  TODO: Correctly read IPv4?
    // for (int i = 0; i < esp_netif_ap_sta_list.num; i++) {
    //     if (esp_netif_ap_sta_list.sta[i].ip.addr == client_addr.sin6_addr.un.u32_addr[3]) return ESP_OK;
    // }

    // //_auth_error:
    // httpd_resp_set_status(req, "401"); // Unauthorized
    // char *unauthorized = "401 Unauthorized - Configured to only accept connections from hotspot devices";
    // httpd_resp_send(req, unauthorized, strlen(unauthorized));
    return ESP_FAIL;
}

static esp_err_t check_auth(HTTPRequest *req, HTTPResponse *res)
{
    if (auth_method == AUTH_METHOD_HOTSPOT)
        return hotspot_auth(req);
    if (auth_method == AUTH_METHOD_BASIC)
        return basic_auth(req, res);
    return ESP_OK;
}
/*
static esp_err_t log_get_handler(httpd_req_t *req) {
    if (check_auth(req) == ESP_FAIL) return ESP_FAIL;

    httpd_resp_set_type(req, "text/plain");

    size_t length;
    void *log_data = log_receive(&length, 1);
    if (log_data == NULL) {
        httpd_resp_sendstr(req, "");

        return ESP_OK;
    }

    httpd_resp_send(req, log_data, length);

    log_return(log_data);

    return ESP_OK;
}

static esp_err_t core_dump_get_handler(httpd_req_t *req) {
    if (check_auth(req) == ESP_FAIL) return ESP_FAIL;

    size_t core_dump_size = core_dump_available();
    if (core_dump_size == 0) {
        httpd_resp_sendstr(req, "No core dump available");
        return ESP_OK;
    }

    httpd_resp_set_type(req, "application/octet-stream");

    const esp_app_desc_t *app_desc = esp_ota_get_app_description();

    char elf_sha256[7];
    esp_ota_get_app_elf_sha256(elf_sha256, sizeof(elf_sha256));

    time_t t = time(NULL);
    char date[20] = "\0";
    if (t > 315360000l) strftime(date, sizeof(date), "_%F_%T", localtime(&t));

    char content_disposition[128];
    snprintf(content_disposition, sizeof(content_disposition),
            "attachment; filename=\"esp32_xbee_%s_core_dump_%s%s.bin\"", app_desc->version, elf_sha256, date);
    httpd_resp_set_hdr(req, "Content-Disposition", content_disposition);

    for (int offset = 0; offset < core_dump_size; offset += BUFFER_SIZE) {
        size_t read = core_dump_size - offset;
        if (read > BUFFER_SIZE) read = BUFFER_SIZE;

        core_dump_read(offset, buffer, read);
        httpd_resp_send_chunk(req, buffer, read);
    }

    httpd_resp_send_chunk(req, NULL, 0);

    return ESP_OK;
}
*/
static void heap_info_get_handler(HTTPRequest *request, HTTPResponse *response)
{
    if (check_auth(request, response) == ESP_FAIL)
    {
        response->setStatusCode(401);
        return;
    }
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);
    DynamicJsonDocument doc(1024);
    JsonObject root = doc.to<JsonObject>();
    root["total_free_bytes"] = info.total_free_bytes;
    root["total_allocated_bytes"] = info.total_allocated_bytes;
    root["largest_free_block"] = info.largest_free_block;
    root["minimum_free_bytes"] = info.minimum_free_bytes;
    root["allocated_blocks"] = info.allocated_blocks;
    root["free_blocks"] = info.free_blocks;
    root["total_blocks"] = info.total_blocks;
    response->setHeader("Content-Type", "application/json");
        response->print(doc.as<String>());

    return;
}

static void handleSdFs(HTTPRequest *request, HTTPResponse *response)
{
    
    if (check_auth(request, response) == ESP_FAIL)
    {
        response->setStatusCode(401);
        return;
    }
  // We only handle GET here
  if (request->getMethod() == "GET") {
    // Redirect / to /index.html
    std::string reqFile = request->getRequestString()=="/" ? "/index.html" : request->getRequestString();

    // Try to open the file
    std::string filename = std::string(DIR_PUBLIC) + reqFile;

    // Check if the file exists
    if (!SD.exists(filename.c_str())) {
      // Send "404 Not Found" as response, as the file doesn't seem to exist
      response->setStatusCode(404);
      response->setStatusText("Not found");
      response->println("404 Not Found");
      return;
    }

    File file = SD.open(filename.c_str());

    // Set length
    response->setHeader("Content-Length", httpsserver::intToString(file.size()));

    // Content-Type is guessed using the definition of the contentTypes-table defined above
    int cTypeIdx = 0;
    do {
      if(reqFile.rfind(contentTypes[cTypeIdx][0])!=std::string::npos) {
        response->setHeader("Content-Type", contentTypes[cTypeIdx][1]);
        break;
      }
      cTypeIdx+=1;
    } while(strlen(contentTypes[cTypeIdx][0])>0);

    // Read the file and write it to the response
    uint8_t buffer[256];
    size_t length = 0;
    do {
      length = file.read(buffer, 256);
      response->write(buffer, length);
    } while (length > 0);

    file.close();
  } else {
    // If there's any body, discard it
    request->discardRequestBody();
    // Send "405 Method not allowed" as response
    response->setStatusCode(405);
    response->setStatusText("Method not allowed");
    response->println("405 Method not allowed");
  }
    return;
}

void printJson(JsonObject v)
{
    char *output[2048];
    serializeJsonPretty(v, output, 2048);
    printf("%s", output);
}

static void config_get_handler(HTTPRequest *request, HTTPResponse *response)
{
    if (check_auth(request, response) == ESP_FAIL){
        response->setStatusCode(401);
    return;
}
    DynamicJsonDocument doc(4096);
    JsonObject root = doc.to<JsonObject>();

    // const esp_app_desc_t *app_desc = esp_ota_get_app_description();
    // root[ "version", app_desc->version];
    char *string;
    string = (char *)calloc(1, 100);

    for (ConfigItem item : CONFIG_ITEMS)
    {
        Serial.println();
        Serial.print(item.key);
        int64_t int64 = 0;
        uint64_t uint64 = 0;

        size_t length = 0;

        String value = "";
        config_color_t color;
        config_ip_addr_t ip(0, 0, 0, 0);
        uint8_t *blobvalue = NULL;
        if (item.type == CONFIG_ITEM_TYPE_STRING)
        {
            value = NVS.getString(item.key);
            root[item.key] = item.secret ? CONFIG_VALUE_UNCHANGED : value;
            Serial.print("  " + value + "  ");
            Serial.print(root.containsKey(item.key));
            Serial.print(root[item.key].is<const char *>());
            //             const char* bb=root[item.key];
            // printf("%s\n",bb);
            // this doesn't work here even if it does work on pc
        }
        else if (item.type == CONFIG_ITEM_TYPE_BLOB)
        {
            // Get length
            length = NVS.getBlobSize(item.key);
            // Get value

            NVS.getBlob(item.key, (uint8_t *)string, length);
            string[length] = '\0';

            root[item.key] = item.secret ? CONFIG_VALUE_UNCHANGED : string;
            // this is most likely not going to work, as a blob may contain characters
            // unsuited for json.
            // this should be base64 encoded
        }
        else if (item.type == CONFIG_ITEM_TYPE_COLOR)
        {
            color.rgba = (uint32_t)NVS.getInt(item.key);
            Serial.print("color gelezen uit nvs ");
            Serial.println(color.rgba);
            printf("#%02x%02x%02x\n", color.values.red, color.values.green, color.values.blue);

            // Convert to hex
            char *stringg;
            asprintf(&stringg, "#%02x%02x%02x", color.values.red, color.values.green, color.values.blue);
            Serial.print(stringg);
            Serial.println(color.values.blue);
            value = item.secret ? CONFIG_VALUE_UNCHANGED : stringg;
            root[item.key] = value;
            Serial.print(value);
            free(stringg);
        }
        else if (item.type == CONFIG_ITEM_TYPE_IP)
        {
            ip.addr = (uint32_t)NVS.getInt(item.key);
            Serial.print("IP gelezen uit nvs  ");
            Serial.print(ip.bytes[0]);
            Serial.print("  ");
            Serial.println(ip.addr);

            root.createNestedArray(item.key);
            // printJson(root);
            for (int b = 0; b < 4; b++)
            {
                root[item.key][b] = ip.bytes[b];
            }
            // printJson(root);
            Serial.print("ip adres 2de byte : ");
            uint8_t bb = root[item.key][0];
            printf("via print byte 0: %i\n", bb);
        }
        else if (item.type == CONFIG_ITEM_TYPE_UINT8 || item.type == CONFIG_ITEM_TYPE_UINT16 || item.type == CONFIG_ITEM_TYPE_UINT32 || item.type == CONFIG_ITEM_TYPE_UINT64)
        {
            uint64 = NVS.getInt(item.key);
            Serial.print("uint64 waarde van " + item.key + "  ");
            Serial.println(uint64);
            // asprintf(&string, "%llu", uint64);
            if (item.secret)
            {
                root[item.key] = CONFIG_VALUE_UNCHANGED;
            }
            else
            {
                root[item.key] = uint64;
            }
        }
        else if (item.type == CONFIG_ITEM_TYPE_BOOL || item.type == CONFIG_ITEM_TYPE_INT8 || item.type == CONFIG_ITEM_TYPE_INT16 || item.type == CONFIG_ITEM_TYPE_INT32 || item.type == CONFIG_ITEM_TYPE_INT64)
        {
            Serial.print("int64 waarde van " + item.key + "  ");

            int64 = NVS.getInt(item.key);
            Serial.println(int64);
            // asprintf(&string, "%lld", int64);
            if (item.secret)
            {
                root[item.key] = CONFIG_VALUE_UNCHANGED;
            }
            else
            {
                root[item.key] = int64;
            }
        }
        else
        {
        }
    }

    // printJson(root);
    response->setHeader("Content-Type", "application/json");
    response->print(doc.as<String>());
    return;
}

void config_post_handler(HTTPRequest *request, HTTPResponse *response)
{
    Serial.println("is in de verwerking geraakt");
    DynamicJsonDocument doc(4096);
    int capacity = 4095;
    // Create buffer to read request
    char *buffer = new char[4096];
    memset(buffer, 0, 4096);

    // Try to read request into buffer
    size_t idx = 0;
    // while "not everything read" or "buffer is full"
    while (!request->requestComplete() && idx < capacity)
    {
        idx += request->readChars(buffer + idx, capacity - idx);
    }

    // If the request is still not read completely, we cannot process it.
    if (!request->requestComplete())
    {
        response->setStatusCode(413);
        response->setStatusText("Request entity too large");
        response->println("413 Request entity too large");
        // Clean up
        delete[] buffer;
        return;
    }

    // Parse the object
    //   JsonObject& reqObj = doc.parseObject(buffer);

    Serial.println("en er is een tempobject");

    DeserializationError error = deserializeJson(doc, buffer);
    if (!error)
    {
        JsonVariant json = doc.as<JsonVariant>();

        for (ConfigItem item : CONFIG_ITEMS)
        {
            Serial.println(item.key);
            if (json[item.key])
            {
                Serial.println(json[item.key].as<String>());
                String strval = "";
                switch (item.type)
                {
                case CONFIG_ITEM_TYPE_STRING:
                case CONFIG_ITEM_TYPE_BLOB:
                    strval = json[item.key].as<String>();
                    if ((strval != "") && (strval != CONFIG_VALUE_UNCHANGED))
                    {
                        if (item.type == CONFIG_ITEM_TYPE_STRING)
                        {
                            NVS.setString(item.key, strval);
                        }
                        else
                        {
                            NVS.setBlob(item.key, (uint8_t *)strval.c_str(), strval.length());
                        }
                    }
                    break;
                case CONFIG_ITEM_TYPE_COLOR:
                {
                    strval = json[item.key].as<String>();
                    config_color_t color;
                    if (strval == "#000000")
                    {
                        color.rgba = 0;
                    }
                    else
                    {
                        // remove the # at position 0 and add the default alpha value at the end
                        strval = strval.substring(1) + "55";
                        color.rgba = std::stoul(strval.c_str(), 0, 16);
                    }
                    NVS.setInt(item.key, color.rgba);
                    break;
                }
                case CONFIG_ITEM_TYPE_IP:
                {
                    config_ip_addr_t ip(0, 0, 0, 0);
                    for (int i = 0; i < 4; i++)
                    {
                        ip.bytes[i] = json[item.key][i];
                    }
                    Serial.print("schrijven ip");
                    Serial.println(ip.addr);
                    NVS.setInt(item.key, ip.addr);
                    break;
                }
                case CONFIG_ITEM_TYPE_BOOL:
                    NVS.setInt(item.key, json[item.key].as<int8_t>());
                    break;
                case CONFIG_ITEM_TYPE_INT8:
                    NVS.setInt(item.key, json[item.key].as<int8_t>());
                    break;
                case CONFIG_ITEM_TYPE_INT16:
                    NVS.setInt(item.key, json[item.key].as<int16_t>());
                    break;
                case CONFIG_ITEM_TYPE_INT32:
                    NVS.setInt(item.key, json[item.key].as<int32_t>());
                    break;
                case CONFIG_ITEM_TYPE_INT64:
                    NVS.setInt(item.key, json[item.key].as<int64_t>());
                    break;
                case CONFIG_ITEM_TYPE_UINT8:
                    NVS.setInt(item.key, json[item.key].as<uint8_t>());
                    break;
                case CONFIG_ITEM_TYPE_UINT16:
                    NVS.setInt(item.key, json[item.key].as<uint16_t>());
                    break;
                case CONFIG_ITEM_TYPE_UINT32:
                    NVS.setInt(item.key, json[item.key].as<uint32_t>());
                    break;
                case CONFIG_ITEM_TYPE_UINT64:
                    NVS.setInt(item.key, json[item.key].as<uint64_t>());
                    break;

                default:
                    break;
                }
            }
        }
        NVS.commit();
        // config_restart();
        StaticJsonDocument<200> docres;
        JsonObject root2 = docres.to<JsonObject>();
        root2["success"] = true;

        response->print(docres.as<String>());
        return;
    }
    // If there's any body, discard it
    request->discardRequestBody();
    // Send "405 Method not allowed" as response
    response->setStatusCode(500);
    response->setStatusText("The posted content is not a valid json");
    response->println("405 Method not allowed");

    return;
}

static void status_get_handler(HTTPRequest *request, HTTPResponse *response)
{
    if (check_auth(request, response) == ESP_FAIL)
    {
        response->setStatusCode(401);
        return;
    }

    DynamicJsonDocument doc(4096);
    JsonObject root = doc.to<JsonObject>();

    // Uptime
    root["uptime"] = (int)((double)esp_timer_get_time() / 1000000);
    // Heap

    root["heap"]["total"] = heap_caps_get_total_size(MALLOC_CAP_8BIT);
    root["heap"]["free"] = heap_caps_get_free_size(MALLOC_CAP_8BIT);

    /*  // Streams
     cJSON *streams = cJSON_AddObjectToObject(root, "streams");
     stream_stats_values_t values;
     for (stream_stats_handle_t stats = stream_stats_first(); stats != NULL; stats = stream_stats_next(stats))
     {
         stream_stats_values(stats, &values);

         cJSON *stream = cJSON_AddObjectToObject(streams, values.name);
         cJSON *total = cJSON_AddObjectToObject(stream, "total");
         cJSON_AddNumberToObject(total, "in", values.total_in);
         cJSON_AddNumberToObject(total, "out", values.total_out);
         cJSON *rate = cJSON_AddObjectToObject(stream, "rate");
         cJSON_AddNumberToObject(rate, "in", values.rate_in);
         cJSON_AddNumberToObject(rate, "out", values.rate_out);
     }

     // WiFi
     wifi_ap_status_t ap_status;
     wifi_sta_status_t sta_status;

     wifi_ap_status(&ap_status);
     wifi_sta_status(&sta_status);

     cJSON *wifi = cJSON_AddObjectToObject(root, "wifi");

     cJSON *ap = cJSON_AddObjectToObject(wifi, "ap");
     cJSON_AddBoolToObject(ap, "active", ap_status.active);
     if (ap_status.active)
     {
         cJSON_AddStringToObject(ap, "ssid", (char *)ap_status.ssid);
         cJSON_AddStringToObject(ap, "authmode", wifi_auth_mode_name(ap_status.authmode));
         cJSON_AddNumberToObject(ap, "devices", ap_status.devices);

         char ip[40];
         snprintf(ip, sizeof(ip), IPSTR, IP2STR(&ap_status.ip4_addr));
         cJSON_AddStringToObject(ap, "ip4", ip);
         snprintf(ip, sizeof(ip), IPV6STR, IPV62STR(ap_status.ip6_addr));
         cJSON_AddStringToObject(ap, "ip6", ip);
     }

     cJSON *sta = cJSON_AddObjectToObject(wifi, "sta");
     cJSON_AddBoolToObject(sta, "active", ap_status.active);
     if (sta_status.active)
     {
         cJSON_AddBoolToObject(sta, "connected", sta_status.connected);
         if (sta_status.connected)
         {
             cJSON_AddStringToObject(sta, "ssid", (char *)sta_status.ssid);
             cJSON_AddStringToObject(sta, "authmode", wifi_auth_mode_name(sta_status.authmode));
             cJSON_AddNumberToObject(sta, "rssi", sta_status.rssi);

             char ip[40];
             snprintf(ip, sizeof(ip), IPSTR, IP2STR(&sta_status.ip4_addr));
             cJSON_AddStringToObject(sta, "ip4", ip);
             snprintf(ip, sizeof(ip), IPV6STR, IPV62STR(sta_status.ip6_addr));
             cJSON_AddStringToObject(sta, "ip6", ip);
         }
     }
    */
    response->setHeader("Content-Type", "application/json");
  
        response->print(doc.as<String>());
    return;
}

/*
static esp_err_t wifi_scan_get_handler(httpd_req_t *req)
{
    if (check_auth(req) == ESP_FAIL)
        return ESP_FAIL;

    uint16_t ap_count;
    wifi_ap_record_t *ap_records = wifi_scan(&ap_count);

    cJSON *root = cJSON_CreateArray();
    for (int i = 0; i < ap_count; i++)
    {
        wifi_ap_record_t *ap_record = &ap_records[i];
        cJSON *ap = cJSON_CreateObject();
        cJSON_AddItemToArray(root, ap);
        cJSON_AddStringToObject(ap, "ssid", (char *)ap_record->ssid);
        cJSON_AddNumberToObject(ap, "rssi", ap_record->rssi);
        cJSON_AddStringToObject(ap, "authmode", wifi_auth_mode_name(ap_record->authmode));
    }

    free(ap_records);

    return json_response(req, root);
}
*/
/* void handleBodyConfig(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
    Serial.println("in de body verwerking");
    if (total > 0 && request->_tempObject == NULL && total < 2048)
    {
        request->_tempObject = malloc(total);
    }
    if (request->_tempObject != NULL)
    {
        memcpy((uint8_t *)(request->_tempObject) + index, data, len);
    }
} */

void WebserverTask(void *server) {
    HTTPServer * myServer = (HTTPServer*) server;
    while(true) {
        myServer->loop();
        delay(1);
    }
}
void WebServices::begin(HTTPServer *server)
{
    /// make sure to start webserial first, because otherwise the last catch all here will probably block the webserial handler


    if (NVS.getInt(CONFIG_ITEMS[CONFIG_ADMIN_AUTH].key) == AUTH_METHOD_BASIC)
    {
    };
    {
        String dummy = "Basic " + base64::encode(NVS.getString(CONFIG_ITEMS[CONFIG_ADMIN_USERNAME].key) + ":" + NVS.getString(CONFIG_ITEMS[CONFIG_ADMIN_PASSWORD].key));
        basic_authentication = (char *)calloc(1, dummy.length() + 1);
        basic_authentication = dummy.c_str();
    }

    ResourceNode *ConfigGetNode = new ResourceNode("/config", "GET", &config_get_handler);
    server->registerNode(ConfigGetNode);
    // server->on("/config", HTTP_GET, config_get_handler);
    ResourceNode *ConfigPostNode = new ResourceNode("/config", "POST", &config_post_handler);
    server->registerNode(ConfigPostNode);
    // server->on("/config", HTTP_POST, config_post_handler, NULL, handleBodyConfig);
    /*   AsyncCallbackJsonWebHandler *config_post_handler_json = new AsyncCallbackJsonWebHandler("/config", [](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                              {
                                                                                                  Serial.println("hier komt hij wel");

                                                                                                  // ...
                                                                                              });
      server->addHandler(config_post_handler_json);
   */
    ResourceNode *StatusGetNode = new ResourceNode("/status", "GET", &status_get_handler);
    server->registerNode(StatusGetNode);

    // server->on("/status", HTTP_GET, status_get_handler);

    // server->on("/log", HTTP_GET, log_get_handler);
    // server->on("/core_dump", HTTP_GET, core_dump_get_handler);
    ResourceNode *HeapInfoGetNode = new ResourceNode("/heapinfo", "GET", &heap_info_get_handler);
    server->registerNode(HeapInfoGetNode);

    // server->on("/heap_info", HTTP_GET, heap_info_get_handler);

    // server->on("/wifi/scan", HTTP_GET, wifi_scan_get_handler);

    //     server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    //   request->send(SD, "/www/index.htm", "text/html");
    // });
    // server->serveStatic("/sd/", SD, "/");
    // server->serveStatic("/", SD, "/");
    
    ResourceNode *sdfsNode = new ResourceNode("", "", &handleSdFs);
    server->setDefaultNode(sdfsNode);
    server->start();
  xTaskCreatePinnedToCore(WebserverTask, "http80", 6144, server, 1, NULL, 0);
}

WebServices webServices;