/* brickpico.h
   Copyright (C) 2021-2023 Timo Kokkonen <tjko@iki.fi>

   SPDX-License-Identifier: GPL-3.0-or-later

   This file is part of BrickPico.

   BrickPico is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   BrickPico is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with BrickPico. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef BRICKPICO_H
#define BRICKPICO_H 1

#include "config.h"
#include "log.h"
#include <time.h>
#include "pico/mutex.h"
#ifdef LIB_PICO_CYW43_ARCH
#define WIFI_SUPPORT 1
#include "lwip/ip_addr.h"
#endif

#include "ringbuffer.h"

#ifndef BRICKPICO_MODEL
#error unknown board model
#endif

#define OUTPUT_MAX_COUNT       16   /* Max number of PWM outputs on the board */

#define MAX_NAME_LEN           64
#define MAX_MAP_POINTS         32
#define MAX_GPIO_PINS          32

#define WIFI_SSID_MAX_LEN      32
#define WIFI_PASSWD_MAX_LEN    64
#define WIFI_COUNTRY_MAX_LEN   3

#define MQTT_MAX_TOPIC_LEN            32
#define MQTT_MAX_USERNAME_LEN         80
#define MQTT_MAX_PASSWORD_LEN         64
#define DEFAULT_MQTT_STATUS_INTERVAL  600
#define DEFAULT_MQTT_TEMP_INTERVAL    60
#define DEFAULT_MQTT_PWR_INTERVAL     600
#define DEFAULT_MQTT_PWM_INTERVAL     600


#ifdef NDEBUG
#define WATCHDOG_ENABLED       1
#define WATCHDOG_REBOOT_DELAY  15000
#endif

#define MAX_EVENT_NAME_LEN     30
#define MAX_EVENT_COUNT        20

enum timer_action_types {
	ACTION_NONE = 0,
	ACTION_ON = 1,
	ACTION_OFF = 2,
};

struct timer_event {
	char name[MAX_EVENT_NAME_LEN];
	int8_t minute;          /* 0-59 */
	int8_t hour;            /* 0-23 */
	uint8_t wday;            /* bitmask for weekdays */
	enum timer_action_types action;
	uint16_t mask;          /* bitmask of outputs this applies to */
};

struct pwm_output {
	char name[MAX_NAME_LEN];

	/* output PWM signal settings */
	uint8_t min_pwm;
	uint8_t max_pwm;
	uint8_t default_pwm;   /* 0..100 (PWM duty cycle) */
	uint8_t default_state; /* 0 = off, 1 = on */
	uint8_t type; /* 0 = Dimmer, 1 = Toggle (on/off) */
};

struct brickpico_config {
	struct pwm_output outputs[OUTPUT_MAX_COUNT];
	bool local_echo;
	uint8_t led_mode;
	char display_type[64];
	char display_theme[16];
	char display_logo[16];
	char display_layout_r[64];
	char name[32];
	char timezone[64];
	bool spi_active;
	bool serial_active;
	uint pwm_freq;
	struct timer_event events[MAX_EVENT_COUNT];
	uint8_t event_count;
	double adc_ref_voltage;
	double temp_offset;
	double temp_coefficient;
#ifdef WIFI_SUPPORT
	char wifi_ssid[WIFI_SSID_MAX_LEN + 1];
	char wifi_passwd[WIFI_PASSWD_MAX_LEN + 1];
	char wifi_country[WIFI_COUNTRY_MAX_LEN + 1];
	uint8_t wifi_mode;
	char hostname[32];
	ip_addr_t syslog_server;
	ip_addr_t ntp_server;
	ip_addr_t ip;
	ip_addr_t netmask;
	ip_addr_t gateway;
	char mqtt_server[32];
	uint32_t mqtt_port;
	bool mqtt_tls;
	bool mqtt_allow_scpi;
	char mqtt_user[MQTT_MAX_USERNAME_LEN + 1];
	char mqtt_pass[MQTT_MAX_PASSWORD_LEN + 1];
	char mqtt_cmd_topic[MQTT_MAX_TOPIC_LEN + 1];
	char mqtt_resp_topic[MQTT_MAX_TOPIC_LEN + 1];
	char mqtt_err_topic[MQTT_MAX_TOPIC_LEN + 1];
	char mqtt_warn_topic[MQTT_MAX_TOPIC_LEN + 1];
	char mqtt_status_topic[MQTT_MAX_TOPIC_LEN + 1];
	char mqtt_pwm_topic[MQTT_MAX_TOPIC_LEN + 1];
	char mqtt_temp_topic[MQTT_MAX_TOPIC_LEN + 1];
	uint32_t mqtt_status_interval;
	uint32_t mqtt_pwm_interval;
	uint32_t mqtt_temp_interval;
	uint16_t mqtt_pwm_mask;
	bool telnet_active;
	bool telnet_auth;
	bool telnet_raw_mode;
	uint32_t telnet_port;
	char telnet_user[16 + 1];
	char telnet_pwhash[128 + 1];
#endif
};

struct brickpico_state {
	/* outputs */
	uint8_t pwm[OUTPUT_MAX_COUNT];
	uint8_t pwr[OUTPUT_MAX_COUNT];
	double temp;
};


struct persistent_memory_block {
	uint32_t id;
	datetime_t saved_time;
	uint64_t uptime;
	uint64_t prev_uptime;
	u8_ringbuffer_t log_rb;
	uint32_t crc32;
	uint8_t log[8192];
};



/* brickpico.c */
extern u8_ringbuffer_t *log_rb;
extern struct persistent_memory_block *persistent_mem;
extern struct brickpico_state *brickpico_state;
extern bool rebooted_by_watchdog;
extern mutex_t *pmem_mutex;
extern mutex_t *state_mutex;
void update_persistent_memory_crc();
void update_persistent_memory();
void update_display_state();
void update_core1_state();

/* bi_decl.c */
void set_binary_info();

/* command.c */
void process_command(struct brickpico_state *state, struct brickpico_config *config, char *command);
int cmd_version(const char *cmd, const char *args, int query, char *prev_cmd);
int last_command_status();

/* config.c */
extern mutex_t *config_mutex;
extern const struct brickpico_config *cfg;
void read_config(bool multicore);
void save_config(bool multicore);
void delete_config(bool multicore);
void print_config();

/* display.c */
void display_init();
void clear_display();
void display_message(int rows, const char **text_lines);
void display_status(const struct brickpico_state *state, const struct brickpico_config *config);

/* display_oled.c */
void oled_display_init();
void oled_clear_display();
void oled_display_status(const struct brickpico_state *state, const struct brickpico_config *conf);
void oled_display_message(int rows, const char **text_lines);

/* flash.h */
int flash_read_file(char **bufptr, uint32_t *sizeptr, const char *filename, int init_flash);
int flash_write_file(const char *buf, uint32_t size, const char *filename);
int flash_delete_file(const char *filename);
int flash_get_fs_info(size_t *size, size_t *free, size_t *files,
		size_t *directories, size_t *filesizetotal);

/* network.c */
void network_init();
void network_mac();
void network_poll();
void network_status();
void set_pico_system_time(long unsigned int sec);
const char *network_ip();
const char *network_hostname();

#if WIFI_SUPPORT

/* httpd.c */
void brickpico_setup_http_handlers();

/* tcpserver.c */
void tcpserver_init();

/* mqtt.c */
void brickpico_setup_mqtt_client();
int brickpico_mqtt_client_active();
void brickpico_mqtt_reconnect();
void brickpico_mqtt_publish();
void brickpico_mqtt_publish_duty();
void brickpico_mqtt_publish_temp();
void brickpico_mqtt_scpi_command();

#endif

/* tls.c */
int read_pem_file(char *buf, uint32_t size, uint32_t timeout, bool append);
#ifdef WIFI_SUPPORT
struct altcp_tls_config* tls_server_config();
#endif

/* pwm.c */
void setup_pwm_inputs();
void setup_pwm_outputs();
void set_pwm_duty_cycle(uint fan, float duty);
float get_pwm_duty_cycle(uint fan);
void get_pwm_duty_cycles(const struct brickpico_config *config);


/* log.c */
int str2log_priority(const char *pri);
const char* log_priority2str(int pri);
int str2log_facility(const char *facility);
const char* log_facility2str(int facility);
void log_msg(int priority, const char *format, ...);
int get_debug_level();
void set_debug_level(int level);
int get_log_level();
void set_log_level(int level);
int get_syslog_level();
void set_syslog_level(int level);
void debug(int debug_level, const char *fmt, ...);

/* timer.c */
int parse_timer_event_str(const char *str, struct timer_event *event);
const char* timer_event_str(const struct timer_event *event);
int handle_timer_events(const struct brickpico_config *conf, struct brickpico_state *state);
const char* timer_action_type_str(enum timer_action_types type);

/* util.c */
void print_mallinfo();
char *trim_str(char *s);
int str_to_int(const char *str, int *val, int base);
int str_to_float(const char *str, float *val);
int str_to_datetime(const char *str, datetime_t *t);
char* datetime_str(char *buf, size_t size, const datetime_t *t);
const char *mac_address_str_r(const uint8_t *mac, char *buf, size_t buf_len);
const char *mac_address_str(const uint8_t *mac);
int valid_wifi_country(const char *country);
int valid_hostname(const char *hostname);
int check_for_change(double oldval, double newval, double threshold);
int64_t pow_i64(int64_t x, uint8_t y);
double round_decimal(double val, unsigned int decimal);
char* base64encode(const char *input);
char* base64decode(const char *input);
char *strncopy(char *dst, const char *src, size_t size);
char *strncatenate(char *dst, const char *src, size_t size);
datetime_t *tm_to_datetime(const struct tm *tm, datetime_t *t);
struct tm *datetime_to_tm(const datetime_t *t, struct tm *tm);
time_t datetime_to_time(const datetime_t *datetime);
int clamp_int(int val, int min, int max);
void* memmem(const void *haystack, size_t haystacklen, const void *needle, size_t needlelen);
char *bitmask_to_str_r(uint32_t mask, uint8_t len, uint8_t base, bool range, char *buf, size_t buf_len);
char *bitmask_to_str(uint32_t mask, uint8_t len, uint8_t base, bool range);
int str_to_bitmask(const char *str, uint8_t len, uint32_t *mask, uint8_t base);

/*  util_rp2040.c */
uint32_t get_stack_pointer();
uint32_t get_stack_free();
void print_rp2040_flashinfo();
void print_rp2040_meminfo();
void print_irqinfo();
void watchdog_disable();
const char *rp2040_model_str();
const char *pico_serial_str();
int time_passed(absolute_time_t *t, uint32_t us);
int getstring_timeout_ms(char *str, uint32_t maxlen, uint32_t timeout);

/* temp.c */
double get_temperature(double adc_ref_voltage, double temp_offset, double temp_coefficient);
void update_temp(const struct brickpico_config *conf, struct brickpico_state *state);

/* crc32.c */
unsigned int xcrc32 (const unsigned char *buf, int len, unsigned int init);


#endif /* BRICKPICO_H */
