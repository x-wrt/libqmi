/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * qmi-firmware-update -- Command line tool to update firmware in QMI devices
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2016 Zodiac Inflight Innovations
 * Copyright (C) 2016-2022 Aleksander Morgado <aleksander@aleksander.es>
 */

#include "config.h"
#include <stdlib.h>
#include "qfu-helpers.h"

/******************************************************************************/

static const gchar *device_type_str[] = {
    [QFU_HELPERS_DEVICE_TYPE_TTY]     = "tty",
    [QFU_HELPERS_DEVICE_TYPE_CDC_WDM] = "cdc-wdm",
};

G_STATIC_ASSERT (G_N_ELEMENTS (device_type_str) == QFU_HELPERS_DEVICE_TYPE_LAST);

const gchar *
qfu_helpers_device_type_to_string (QfuHelpersDeviceType type)
{
    return device_type_str[type];
}
