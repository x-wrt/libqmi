# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2019 - 2021 Iñigo Martinez <inigomartinez@gmail.com>

help2man = find_program('help2man')

mans = [
  ['qmicli', [help2man, '--output=@OUTPUT@', '--name=Control QMI devices', '--help-option="--help-all"', qmicli]],
  ['qmi-network', [help2man, '--output=@OUTPUT@', '--name=Simple network management of QMI devices', qmi_network]],
]

if enable_firmware_update
  mans += [['qmi-firmware-update', [help2man, '--output=@OUTPUT@', '--name=Update firmware in QMI devices', qmi_firmware_update]]]
endif

foreach man: mans
  custom_target(
    man[0],
    output: man[0] + '.1',
    command: man[1],
    install: true,
    install_dir: qmi_mandir / 'man1',
  )
endforeach
