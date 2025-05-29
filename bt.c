#if defined(BLUETOOTH) && !defined(__WIN32) && !defined(__WIN64)
/*
Copyright (c) 2025, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.

Bluetooth transport - BlueZ D-Bus API implementation

This implementation uses BlueZ D-Bus API instead of gattlib for better compatibility
*/

#undef DEBUG_MEM

#include "mybmm.h"
#include "pthread.h"
#include <gio/gio.h>
#include <glib.h>
#include <string.h>
#include <unistd.h>



struct bt_session {
        GDBusConnection *dbus_conn;
        char target[32];
        char topts[128];
        char device_path[256];
        char char_read_path[512];
        char char_write_path[512];
        char data[4096];
        int len;
        int cbcnt;
        int connected;
        int notifications_enabled;
        pthread_mutex_t data_lock;
        GMainLoop *main_loop;
        pthread_t dbus_thread;
};
typedef struct bt_session bt_session_t;

static int bt_init(mybmm_config_t *conf) {
        return 0;
}

// Convert MAC address from AA:BB:CC:DD:EE:FF to AA_BB_CC_DD_EE_FF for D-Bus paths
static void mac_to_dbus_path(const char *mac, char *path_part) {
        strcpy(path_part, mac);
        for (int i = 0; path_part[i]; i++) {
                if (path_part[i] == ':') path_part[i] = '_';
        }
}

static void on_properties_changed(GDBusConnection *connection,
                                const gchar *sender_name,
                                const gchar *object_path,
                                const gchar *interface_name,
                                const gchar *signal_name,
                                GVariant *parameters,
                                gpointer user_data) {
        bt_session_t *s = (bt_session_t *)user_data;
        
        if (strcmp(signal_name, "PropertiesChanged") != 0) return;
        if (!strstr(object_path, s->char_read_path)) return;
        
        const gchar *interface;
        GVariant *changed_properties;
        GVariant *invalidated_properties;
        
        g_variant_get(parameters, "(&s@a{sv}@as)", &interface, &changed_properties, &invalidated_properties);
        
        if (strcmp(interface, "org.bluez.GattCharacteristic1") == 0) {
                GVariant *value_variant = g_variant_lookup_value(changed_properties, "Value", G_VARIANT_TYPE("ay"));
                if (value_variant) {
                        gsize data_len;
                        const guint8 *data = g_variant_get_fixed_array(value_variant, &data_len, sizeof(guint8));
                        
                        dprintf(1,"*** NOTIFICATION RECEIVED! *** Length: %zu\n", data_len);
                        if (debug > 2) bindump("notification data", (void*)data, data_len);
                        
                        pthread_mutex_lock(&s->data_lock);
                        if (s->len + data_len <= sizeof(s->data)) {
                                memcpy(&s->data[s->len], data, data_len);
                                s->len += data_len;
                                s->cbcnt++;
                                dprintf(1,"Added %zu bytes to buffer, total length now: %d\n", data_len, s->len);
                        } else {
                                dprintf(1,"Warning: notification would overflow buffer\n");
                        }
                        pthread_mutex_unlock(&s->data_lock);
                        
                        g_variant_unref(value_variant);
                }
        }
        
        g_variant_unref(changed_properties);
        g_variant_unref(invalidated_properties);
}

// D-Bus main loop thread
static void* dbus_main_loop_thread(void* arg) {
        bt_session_t *s = (bt_session_t *)arg;
        g_main_loop_run(s->main_loop);
        return NULL;
}

static void *bt_new(mybmm_config_t *conf, ...) {
        bt_session_t *s;
        va_list ap;
        char *target, *topts;

        va_start(ap, conf);
        target = va_arg(ap, char *);
        topts = va_arg(ap, char *);
        va_end(ap);

        dprintf(5,"target: %s, topts: %s\n", target, topts);
        
        s = calloc(1, sizeof(*s));
        if (!s) {
                perror("bt_new: malloc");
                return 0;
        }
        
        strncpy(s->target, target, sizeof(s->target) - 1);
        if (topts) {
                strncpy(s->topts, topts, sizeof(s->topts) - 1);
        }
        
        pthread_mutex_init(&s->data_lock, NULL);
        
        dprintf(5,"target: %s, topts: %s\n", s->target, s->topts);
        return s;
}

static int bt_open(void *handle) {
        bt_session_t *s = handle;
        GError *error = NULL;
        char mac_path[64];
        GVariant *result;
        
        dprintf(1,"Connecting to: %s\n", s->target);
        
        if (s->connected) return 0;
        
        // Connect to system D-Bus
        s->dbus_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
        if (!s->dbus_conn) {
                dprintf(1,"Failed to connect to D-Bus: %s\n", error->message);
                g_error_free(error);
                return 1;
        }
        
        dprintf(1,"Connected to D-Bus\n");
        
        // Convert MAC address for D-Bus path
        mac_to_dbus_path(s->target, mac_path);
        snprintf(s->device_path, sizeof(s->device_path), 
                "/org/bluez/hci0/dev_%s", mac_path);
        
        dprintf(1,"Device path: %s\n", s->device_path);
        
        // Connect to the device
        result = g_dbus_connection_call_sync(s->dbus_conn,
                "org.bluez",
                s->device_path,
                "org.bluez.Device1",
                "Connect",
                NULL,
                NULL,
                G_DBUS_CALL_FLAGS_NONE,
                10000, // 10 second timeout
                NULL,
                &error);
                
        if (!result) {
                dprintf(1,"Failed to connect to device: %s\n", error->message);
                g_error_free(error);
                return 1;
        }
        
        g_variant_unref(result);
        dprintf(1,"Device connected successfully\n");
        
        // Wait a moment for services to be discovered
        sleep(2);
        
        // Based on your working Python script and previous debugging:
        // - Read/Notify characteristic: UUID 0xff01, should be at handle 0x0010
        // - Write characteristic: UUID 0xff02, should be at handle 0x0014
        
        // Initialize characteristic paths (will be found via service discovery)
        s->char_read_path[0] = '\0';
        s->char_write_path[0] = '\0';
                
        // Alternative approach: discover services and characteristics
        // Get all managed objects to find the correct characteristic paths
        result = g_dbus_connection_call_sync(s->dbus_conn,
                "org.bluez",
                "/",
                "org.freedesktop.DBus.ObjectManager",
                "GetManagedObjects",
                NULL,
                NULL,
                G_DBUS_CALL_FLAGS_NONE,
                5000,
                NULL,
                &error);
                
        if (result) {
                GVariantIter *objects_iter;
                const gchar *object_path;
                GVariant *interfaces_dict;
                
                g_variant_get(result, "(a{oa{sa{sv}}})", &objects_iter);
                
                while (g_variant_iter_loop(objects_iter, "{&o@a{sa{sv}}}", &object_path, &interfaces_dict)) {
                        // Look for characteristics under our device
                        if (strstr(object_path, s->device_path) && strstr(object_path, "char")) {
                                GVariantIter *interfaces_iter;
                                const gchar *interface_name;
                                GVariant *properties_dict;
                                
                                g_variant_get(interfaces_dict, "a{sa{sv}}", &interfaces_iter);
                                
                                while (g_variant_iter_loop(interfaces_iter, "{&s@a{sv}}", &interface_name, &properties_dict)) {
                                        if (strcmp(interface_name, "org.bluez.GattCharacteristic1") == 0) {
                                                GVariant *uuid_variant = g_variant_lookup_value(properties_dict, "UUID", G_VARIANT_TYPE_STRING);
                                                if (uuid_variant) {
                                                        const gchar *uuid = g_variant_get_string(uuid_variant, NULL);
                                                        dprintf(1,"Found characteristic: %s UUID: %s\n", object_path, uuid);
                                                        
                                                        // Look for our target UUIDs
                                                        if (strstr(uuid, "0000ff01") || strstr(uuid, "ff01")) {
                                                                snprintf(s->char_read_path, sizeof(s->char_read_path), "%s", object_path);
                                                                dprintf(1,"Read characteristic path: %s\n", s->char_read_path);
                                                        } else if (strstr(uuid, "0000ff02") || strstr(uuid, "ff02")) {
                                                                snprintf(s->char_write_path, sizeof(s->char_write_path), "%s", object_path);
                                                                dprintf(1,"Write characteristic path: %s\n", s->char_write_path);
                                                        }
                                                        
                                                        g_variant_unref(uuid_variant);
                                                }
                                        }
                                }
                                g_variant_iter_free(interfaces_iter);
                        }
                }
                g_variant_iter_free(objects_iter);
                g_variant_unref(result);
        }
        
        if (strlen(s->char_read_path) == 0 || strlen(s->char_write_path) == 0) {
                dprintf(1,"Failed to find required characteristics\n");
                return 1;
        }
        
        
        // Set up D-Bus signal monitoring for notifications
        s->main_loop = g_main_loop_new(NULL, FALSE);
        
        guint subscription_id = g_dbus_connection_signal_subscribe(s->dbus_conn,
                "org.bluez",
                "org.freedesktop.DBus.Properties",
                "PropertiesChanged",
                s->char_read_path,
                NULL,
                G_DBUS_SIGNAL_FLAGS_NONE,
                on_properties_changed,
                s,
                NULL);
                
        dprintf(1,"D-Bus signal subscription ID: %u\n", subscription_id);
        
        // Start the D-Bus main loop in a separate thread
        if (pthread_create(&s->dbus_thread, NULL, dbus_main_loop_thread, s) != 0) {
                dprintf(1,"Failed to create D-Bus thread\n");
                return 1;
        }
        
        // Enable notifications on the characteristic
        result = g_dbus_connection_call_sync(s->dbus_conn,
                "org.bluez",
                s->char_read_path,
                "org.bluez.GattCharacteristic1",
                "StartNotify",
                NULL,
                NULL,
                G_DBUS_CALL_FLAGS_NONE,
                5000,
                NULL,
                &error);
                
        if (!result) {
                dprintf(1,"Failed to start notifications: %s\n", error->message);
                g_error_free(error);
                // Continue anyway - some devices work without explicit StartNotify
        } else {
                dprintf(1,"Notifications enabled successfully\n");
                g_variant_unref(result);
                s->notifications_enabled = 1;
        }
        
        s->connected = 1;
        dprintf(1,"BLE connection setup complete\n");
        return 0;
}

static int bt_read(void *handle, ...) {
        bt_session_t *s = handle;
        int len = 0;
        uint8_t *buf;
        int buflen, retries;
        va_list ap;

        if (!s->connected) return -1;

        va_start(ap, handle);
        buf = va_arg(ap, uint8_t *);
        buflen = va_arg(ap, int);
        va_end(ap);

        dprintf(1,"buf: %p, buflen: %d\n", buf, buflen);

        retries = 10;
        while (retries > 0) {
                pthread_mutex_lock(&s->data_lock);
                dprintf(1,"s->len: %d, s->cbcnt: %d\n", s->len, s->cbcnt);
                
                if (s->len > 0) {
                        len = (s->len > buflen) ? buflen : s->len;
                        memcpy(buf, s->data, len);
                        s->len = 0; // Clear buffer after reading
                        dprintf(1,"Copying %d bytes from notification buffer\n", len);
                        pthread_mutex_unlock(&s->data_lock);
                        break;
                }
                pthread_mutex_unlock(&s->data_lock);
                
                retries--;
                if (retries == 0) {
                        dprintf(1,"No notifications received after 10 attempts\n");
                        return 0;
                }
                
                dprintf(1,"Waiting for notifications... (attempt %d)\n", 11 - retries);
                sleep(1);
        }

        return len;
}

static int bt_write(void *handle, ...) {
        bt_session_t *s = handle;
        uint8_t *buf;
        int buflen;
        va_list ap;
        GError *error = NULL;

        if (!s->connected) return -1;

        va_start(ap, handle);
        buf = va_arg(ap, uint8_t *);
        buflen = va_arg(ap, int);
        va_end(ap);

        dprintf(1,"buf: %p, buflen: %d\n", buf, buflen);
        if (debug > 2) bindump("bt write", buf, buflen);

        // Clear notification buffer before writing
        pthread_mutex_lock(&s->data_lock);
        s->len = 0;
        s->cbcnt = 0;
        pthread_mutex_unlock(&s->data_lock);

        // Create GVariant array for the data
        GVariant *data_variant = g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE, 
                                                         buf, buflen, sizeof(guint8));
        
        // Create options dictionary (empty for now, but could include WriteType)
        GVariantBuilder options_builder;
        g_variant_builder_init(&options_builder, G_VARIANT_TYPE("a{sv}"));
        // For write without response (like Python script):
        g_variant_builder_add(&options_builder, "{sv}", "type", g_variant_new_string("command"));
        GVariant *options = g_variant_builder_end(&options_builder);

        // Write to characteristic
        GVariant *result = g_dbus_connection_call_sync(s->dbus_conn,
                "org.bluez",
                s->char_write_path,
                "org.bluez.GattCharacteristic1",
                "WriteValue",
                g_variant_new("(@ay@a{sv})", data_variant, options),
                NULL,
                G_DBUS_CALL_FLAGS_NONE,
                5000,
                NULL,
                &error);

        if (!result) {
                dprintf(1,"Failed to write characteristic: %s\n", error->message);
                g_error_free(error);
                return -1;
        }

        g_variant_unref(result);
        dprintf(1,"Write completed successfully\n");
        
        // Give device time to process and respond
        usleep(100000); // 100ms delay
        
        return buflen;
}

static int bt_close(void *handle) {
        bt_session_t *s = handle;
        GError *error = NULL;

        dprintf(1,"Closing BLE connection\n");
        
        if (!s->connected) return 0;

        // Stop notifications
        if (s->notifications_enabled) {
                GVariant *result = g_dbus_connection_call_sync(s->dbus_conn,
                        "org.bluez",
                        s->char_read_path,
                        "org.bluez.GattCharacteristic1",
                        "StopNotify",
                        NULL,
                        NULL,
                        G_DBUS_CALL_FLAGS_NONE,
                        5000,
                        NULL,
                        &error);
                        
                if (result) {
                        g_variant_unref(result);
                        dprintf(1,"Notifications stopped\n");
                } else {
                        dprintf(1,"Failed to stop notifications: %s\n", error->message);
                        g_error_free(error);
                }
        }
        
        // Stop the main loop and join thread
        if (s->main_loop) {
                g_main_loop_quit(s->main_loop);
                pthread_join(s->dbus_thread, NULL);
                g_main_loop_unref(s->main_loop);
                s->main_loop = NULL;
        }

        // Disconnect from device
        GVariant *result = g_dbus_connection_call_sync(s->dbus_conn,
                "org.bluez",
                s->device_path,
                "org.bluez.Device1",
                "Disconnect",
                NULL,
                NULL,
                G_DBUS_CALL_FLAGS_NONE,
                5000,
                NULL,
                &error);
                
        if (result) {
                g_variant_unref(result);
                dprintf(1,"Device disconnected\n");
        } else {
                dprintf(1,"Failed to disconnect device: %s\n", error->message);
                g_error_free(error);
        }

        // Clean up D-Bus connection
        if (s->dbus_conn) {
                g_object_unref(s->dbus_conn);
                s->dbus_conn = NULL;
        }
        
        pthread_mutex_destroy(&s->data_lock);
        s->connected = 0;
        
        return 0;
}

mybmm_module_t bt_module = {
        MYBMM_MODTYPE_TRANSPORT,
        "bt",
        0,
        bt_init,
        bt_new,
        bt_open,
        bt_read,
        bt_write,
        bt_close
};
#endif
