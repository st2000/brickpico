/* effects_fade.c
   Copyright (C) 2025 Timo Kokkonen <tjko@iki.fi>

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"

#include "brickpico.h"
#include "effects.h"


typedef struct fade_context {
	float fade_in;
	float fade_out;
	int64_t in_l;
	int64_t out_l;
	uint8_t last_state;
	uint8_t mode;
	uint64_t start_t;
} fade_context_t;


void* effect_fade_parse_args(const char *args)
{
	fade_context_t *c;
	char *tok, *saveptr, s[64];
	float arg;

	if (!args)
		return NULL;
	if (!(c = calloc(1, sizeof(fade_context_t))))
		return NULL;

	/* Defaults (seconds) */
	c->fade_in = 1.0;
	c->fade_out = 1.0;

	strncopy(s, args, sizeof(s));

	/* Parse parameters */
	if ((tok = strtok_r(s, ",", &saveptr))) {
		if (str_to_float(tok, &arg)) {
			if (arg >= 0.0)
				c->fade_in = arg;
			if ((tok = strtok_r(NULL, ",", &saveptr))) {
				if (str_to_float(tok, &arg)) {
					if (arg >= 0.0)
						c->fade_out = arg;
				}
			}
		}
	}

	c->in_l = c->fade_in * 1000000;
	c->out_l = c->fade_out * 1000000;
	c->last_state = 0;
	c->mode = 0;
	c->start_t = 0;

	return c;
}

char* effect_fade_print_args(void *ctx)
{
	fade_context_t *c = (fade_context_t*)ctx;
	char buf[64];

	snprintf(buf, sizeof(buf), "%f,%f", c->fade_in, c->fade_out);

	return strdup(buf);
}

uint8_t effect_fade(void *ctx, uint64_t t_now, uint8_t pwm, uint8_t pwr)
{
	fade_context_t *c = (fade_context_t*)ctx;
	int64_t t_d;
	uint8_t ret = 0;

	if (c->last_state != pwr) {
		/* Start fade in/out sequence... */
		c->start_t = t_now;
		if (pwr) {
			c->mode = 1;
			ret = 0;
		} else {
			c->mode = 3;
			ret = pwm;
		}
	}
	else {
		t_d = t_now - c->start_t;

		if (c->mode == 1) { /* Fade in... */
			if (t_d < c->in_l) {
				ret = pwm * t_d / c->in_l;
			} else {
				c->mode = 2;
				ret = pwm;
			}
		}
		else if (c->mode == 2) { /* On state after fade in... */
			ret = pwm;
		}
		else if (c->mode == 3) { /* Fade out... */
			if (t_d < c->out_l) {
				ret = pwm - (pwm * t_d / c->out_l);
			} else {
				c->mode = 4;
				ret = 0;
			}
		}
		else if (c->mode == 4) { /* Off state after fade out... */
			ret = 0;
		}
	}

	c->last_state = pwr;

	return ret;
}


/* eof :-) */
