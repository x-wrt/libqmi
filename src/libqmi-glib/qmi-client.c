/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * libqmi-glib -- GLib/GIO based library to control QMI devices
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 * Copyright (C) 2012-2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <gio/gio.h>

#include "qmi-error-types.h"
#include "qmi-enum-types.h"
#include "qmi-flag-types.h"
#include "qmi-device.h"
#include "qmi-client.h"
#include "qmi-ctl.h"

G_DEFINE_ABSTRACT_TYPE (QmiClient, qmi_client, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_DEVICE,
    PROP_SERVICE,
    PROP_CID,
    PROP_VERSION_MAJOR,
    PROP_VERSION_MINOR,
    PROP_VALID,
    PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

struct _QmiClientPrivate {
    QmiDevice *device;
    QmiService service;
    guint8 cid;
    guint version_major;
    guint version_minor;

    guint16 transaction_id;
};

/*****************************************************************************/

GObject *
qmi_client_get_device (QmiClient *self)
{
    GObject *device;

    g_return_val_if_fail (QMI_IS_CLIENT (self), NULL);

    g_object_get (G_OBJECT (self),
                  QMI_CLIENT_DEVICE, &device,
                  NULL);

    return device;
}

GObject *
qmi_client_peek_device (QmiClient *self)
{
    g_return_val_if_fail (QMI_IS_CLIENT (self), NULL);

    return G_OBJECT (self->priv->device);
}

QmiService
qmi_client_get_service (QmiClient *self)
{
    g_return_val_if_fail (QMI_IS_CLIENT (self), QMI_SERVICE_UNKNOWN);

    return self->priv->service;
}

guint8
qmi_client_get_cid (QmiClient *self)
{
    g_return_val_if_fail (QMI_IS_CLIENT (self), QMI_CID_NONE);

    return self->priv->cid;
}

gboolean
qmi_client_is_valid (QmiClient *self)
{
    g_return_val_if_fail (QMI_IS_CLIENT (self), FALSE);

    return (self->priv->service != QMI_SERVICE_UNKNOWN &&
            QMI_IS_DEVICE (self->priv->device) &&
            ((self->priv->cid != QMI_CID_NONE || self->priv->service == QMI_SERVICE_CTL)));
}

gboolean
qmi_client_get_version (QmiClient *self,
                        guint *major,
                        guint *minor)
{
    g_return_val_if_fail (QMI_IS_CLIENT (self), FALSE);

    /* If the major version is greater than zero, assume it was
     * set properly */
    if (!self->priv->version_major)
        return FALSE;

    *major = self->priv->version_major;
    *minor = self->priv->version_minor;
    return TRUE;
}

gboolean
qmi_client_check_version (QmiClient *self,
                          guint major,
                          guint minor)
{
    g_return_val_if_fail (QMI_IS_CLIENT (self), FALSE);

    /* If the major version is greater than zero, assume it was
     * set properly */
    if (!self->priv->version_major)
        return FALSE;

    if (self->priv->version_major > major ||
        (self->priv->version_major == major &&
         self->priv->version_minor >= minor))
        return TRUE;

    return FALSE;
}

guint16
qmi_client_get_next_transaction_id (QmiClient *self)
{
    guint16 next;

    g_return_val_if_fail (QMI_IS_CLIENT (self), 0);

    next = self->priv->transaction_id;

    /* Don't go further than 8bits in the CTL service */
    if ((self->priv->service == QMI_SERVICE_CTL &&
         self->priv->transaction_id == G_MAXUINT8) ||
        self->priv->transaction_id == G_MAXUINT16)
        /* Reset! */
        self->priv->transaction_id = 0x01;
    else
        self->priv->transaction_id++;

    return next;
}

/*****************************************************************************/

void
__qmi_client_process_indication (QmiClient *self,
                                 QmiMessage *message)
{
    if (QMI_CLIENT_GET_CLASS (self)->process_indication)
        QMI_CLIENT_GET_CLASS (self)->process_indication (self, message);
}

/*****************************************************************************/

static void
set_property (GObject *object,
              guint prop_id,
              const GValue *value,
              GParamSpec *pspec)
{
    QmiClient *self = QMI_CLIENT (object);

    switch (prop_id) {
    case PROP_DEVICE:
        /* NOTE!! We do NOT keep a reference to the device here.
         * Clients are OWNED by the device */
        self->priv->device = g_value_get_object (value);
        break;
    case PROP_SERVICE:
        self->priv->service = g_value_get_enum (value);
        break;
    case PROP_CID:
        self->priv->cid = (guint8)g_value_get_uint (value);
        break;
    case PROP_VERSION_MAJOR:
        self->priv->version_major = g_value_get_uint (value);
        break;
    case PROP_VERSION_MINOR:
        self->priv->version_minor = g_value_get_uint (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject *object,
              guint prop_id,
              GValue *value,
              GParamSpec *pspec)
{
    QmiClient *self = QMI_CLIENT (object);

    switch (prop_id) {
    case PROP_DEVICE:
        g_value_set_object (value, self->priv->device);
        break;
    case PROP_SERVICE:
        g_value_set_enum (value, self->priv->service);
        break;
    case PROP_CID:
        g_value_set_uint (value, (guint)self->priv->cid);
        break;
    case PROP_VERSION_MAJOR:
        g_value_set_uint (value, self->priv->version_major);
        break;
    case PROP_VERSION_MINOR:
        g_value_set_uint (value, self->priv->version_minor);
        break;
    case PROP_VALID:
        g_value_set_boolean (value, qmi_client_is_valid (self));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
qmi_client_init (QmiClient *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self),
                                              QMI_TYPE_CLIENT,
                                              QmiClientPrivate);

    /* Defaults */
    self->priv->service = QMI_SERVICE_UNKNOWN;
    self->priv->transaction_id = 0x01;
    self->priv->cid = QMI_CID_NONE;
    self->priv->version_major = 0;
    self->priv->version_minor = 0;
}

static void
qmi_client_class_init (QmiClientClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (QmiClientPrivate));

    object_class->get_property = get_property;
    object_class->set_property = set_property;

    /**
     * QmiClient:client-device:
     *
     * Since: 1.0
     */
    properties[PROP_DEVICE] =
        g_param_spec_object (QMI_CLIENT_DEVICE,
                             "Device",
                             "The QMI device",
                             QMI_TYPE_DEVICE,
                             G_PARAM_READWRITE);
    g_object_class_install_property (object_class, PROP_DEVICE, properties[PROP_DEVICE]);

    /**
     * QmiClient:client-service:
     *
     * Since: 1.0
     */
    properties[PROP_SERVICE] =
        g_param_spec_enum (QMI_CLIENT_SERVICE,
                           "Service",
                           "QMI service this client is using",
                           QMI_TYPE_SERVICE,
                           QMI_SERVICE_UNKNOWN,
                           G_PARAM_READWRITE);
    g_object_class_install_property (object_class, PROP_SERVICE, properties[PROP_SERVICE]);

    /**
     * QmiClient:client-cid:
     *
     * Since: 1.0
     */
    properties[PROP_CID] =
        g_param_spec_uint (QMI_CLIENT_CID,
                           "Client ID",
                           "ID of the client registered into the QMI device",
                           0,
                           G_MAXUINT8,
                           QMI_CID_NONE,
                           G_PARAM_READWRITE);
    g_object_class_install_property (object_class, PROP_CID, properties[PROP_CID]);

    /**
     * QmiClient:client-version-major:
     *
     * Since: 1.0
     */
    properties[PROP_VERSION_MAJOR] =
        g_param_spec_uint (QMI_CLIENT_VERSION_MAJOR,
                           "Version major",
                           "Major version of the service handled by this client",
                           0,
                           G_MAXUINT,
                           0,
                           G_PARAM_READWRITE);
    g_object_class_install_property (object_class, PROP_VERSION_MAJOR, properties[PROP_VERSION_MAJOR]);

    /**
     * QmiClient:client-version-minor:
     *
     * Since: 1.0
     */
    properties[PROP_VERSION_MINOR] =
        g_param_spec_uint (QMI_CLIENT_VERSION_MINOR,
                           "Version minor",
                           "Minor version of the service handled by this client",
                           0,
                           G_MAXUINT,
                           0,
                           G_PARAM_READWRITE);
    g_object_class_install_property (object_class, PROP_VERSION_MINOR, properties[PROP_VERSION_MINOR]);

    /**
     * QmiClient:client-valid:
     *
     * Since: 1.20
     */
    properties[PROP_VALID] =
        g_param_spec_boolean (QMI_CLIENT_VALID,
                              "Valid",
                              "Whether the client is valid and usable",
                              FALSE,
                              G_PARAM_READABLE);
    g_object_class_install_property (object_class, PROP_VALID, properties[PROP_VALID]);
}
