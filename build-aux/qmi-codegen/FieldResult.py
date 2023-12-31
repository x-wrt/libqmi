# -*- Mode: python; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option) any
# later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program; if not, write to the Free Software Foundation, Inc., 51
# Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
# Copyright (C) 2012 Lanedo GmbH
# Copyright (C) 2012-2022 Aleksander Morgado <aleksander@aleksander.es>
#

import string

import utils
import TypeFactory
from Field import Field


"""
The FieldResult class takes care of handling the common 'Result' TLV
"""
class FieldResult(Field):

    """
    Emit the types required to the source file (they will not be exposed in the
    interface)
    """
    def emit_types(self, hfile, cfile):
        if not TypeFactory.is_type_emitted(self.fullname):
            TypeFactory.set_type_emitted(self.fullname)
            self.variable.emit_types(cfile, cfile, self.since, True)


    """
    Emit the method responsible for getting the Result TLV contents. This
    special TLV will have its own getter implementation, as we want to have
    proper GErrors built from the QMI result status/code.
    """
    def emit_getter(self, hfile, cfile):
        translations = { 'variable_name'     : self.variable_name,
                         'prefix_camelcase'  : utils.build_camelcase_name(self.prefix),
                         'prefix_underscore' : utils.build_underscore_name(self.prefix),
                         'since'             : self.since }

        # Emit the getter header
        template = '\n'
        if not self.static and self.service != 'CTL':
            template += (
                '\n'
                '/**\n'
                ' * ${prefix_underscore}_get_result:\n'
                ' * @self: a ${prefix_camelcase}.\n'
                ' * @error: Return location for error or %NULL.\n'
                ' *\n'
                ' * Get the result of the QMI operation.\n'
                ' *\n'
                ' * Returns: (skip): %TRUE if the QMI operation succeeded, %FALSE if @error is set.\n'
                ' *\n'
                ' * Since: ${since}\n'
                ' */\n')
        template += (
            'gboolean ${prefix_underscore}_get_result (\n'
            '    ${prefix_camelcase} *self,\n'
            '    GError **error);\n')
        hfile.write(string.Template(template).substitute(translations))

        # Emit the getter source
        template = (
            '\n'
            'gboolean\n'
            '${prefix_underscore}_get_result (\n'
            '    ${prefix_camelcase} *self,\n'
            '    GError **error)\n'
            '{\n'
            '    g_return_val_if_fail (self != NULL, FALSE);\n'
            '\n'
            '    /* We should always have a result set in the response message */\n'
            '    if (!self->${variable_name}_set) {\n'
            '        g_set_error (error,\n'
            '                     QMI_CORE_ERROR,\n'
            '                     QMI_CORE_ERROR_INVALID_MESSAGE,\n'
            '                     "No \'Result\' field given in the message");\n'
            '        return FALSE;\n'
            '    }\n'
            '\n'
            '    if (self->${variable_name}_error_status == QMI_STATUS_SUCCESS) {\n'
            '        /* Operation succeeded */\n'
            '        return TRUE;\n'
            '    }\n'
            '\n'
            '    /* Report a QMI protocol error */\n'
            '    g_set_error (error,\n'
            '                 QMI_PROTOCOL_ERROR,\n'
            '                 (QmiProtocolError) self->${variable_name}_error_code,\n'
            '                 "QMI protocol error (%u): \'%s\'",\n'
            '                 self->${variable_name}_error_code,\n'
            '                 qmi_protocol_error_get_string ((QmiProtocolError) self->${variable_name}_error_code));\n'
            '    return FALSE;\n'
            '}\n')
        cfile.write(string.Template(template).substitute(translations))


    """
    Emit the method responsible for getting a printable representation of this
    TLV field.
    """
    def emit_tlv_helpers(self, f):
        if TypeFactory.helpers_emitted(self.fullname):
            return

        TypeFactory.set_helpers_emitted(self.fullname)

        translations = { 'name'       : self.name,
                         'tlv_id'     : self.id_enum_name,
                         'underscore' : utils.build_underscore_name (self.fullname) }

        template = (
            '\n'
            'static gboolean\n'
            '${underscore}_validate (\n'
            '    const guint8 *buffer,\n'
            '    guint16 buffer_len)\n'
            '{\n'
            '    static const guint expected_len = 4;\n'
            '\n'
            '    if (buffer_len < expected_len) {\n'
            '        g_warning ("Cannot read the \'${name}\' TLV: expected \'%u\' bytes, but only got \'%u\' bytes",\n'
            '                   expected_len, buffer_len);\n'
            '        return FALSE;\n'
            '    }\n'
            '\n'
            '    return TRUE;\n'
            '}\n')
        f.write(string.Template(template).substitute(translations))

        template = (
            '\n'
            'static gchar *\n'
            '${underscore}_get_printable (\n'
            '    QmiMessage *self,\n'
            '    const gchar *line_prefix)\n'
            '{\n'
            '    gsize offset = 0;\n'
            '    gsize init_offset;\n'
            '    guint16 error_status;\n'
            '    guint16 error_code;\n'
            '\n'
            '    if ((init_offset = qmi_message_tlv_read_init (self, ${tlv_id}, NULL, NULL)) == 0)\n'
            '        return NULL;\n'
            '    if (!qmi_message_tlv_read_guint16 (self, init_offset, &offset, QMI_ENDIAN_LITTLE, &error_status, NULL))\n'
            '        return NULL;\n'
            '    if (!qmi_message_tlv_read_guint16 (self, init_offset, &offset, QMI_ENDIAN_LITTLE, &error_code, NULL))\n'
            '        return NULL;\n'
            '    g_warn_if_fail (qmi_message_tlv_read_remaining_size (self, init_offset, offset) == 0);\n'
            '\n'
            '    if (error_status == QMI_STATUS_SUCCESS)\n'
            '        return g_strdup ("SUCCESS");\n'
            '\n'
            '    return g_strdup_printf ("FAILURE: %s", qmi_protocol_error_get_string ((QmiProtocolError) error_code));\n'
            '}\n')
        f.write(string.Template(template).substitute(translations))


    """
    Add sections
    """
    def add_sections(self, sections):
        translations = { 'underscore'        : utils.build_underscore_name(self.name),
                         'prefix_camelcase'  : utils.build_camelcase_name(self.prefix),
                         'prefix_underscore' : utils.build_underscore_name(self.prefix) }

        # Public methods
        template = (
            '${prefix_underscore}_get_${underscore}\n')
        sections['public-methods'] += string.Template(template).substitute(translations)
