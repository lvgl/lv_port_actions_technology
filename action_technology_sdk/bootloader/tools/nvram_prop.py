#!/usr/bin/env python3
#
# Actions NVRAM properties helper
#
# Copyright (c) 2017 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0
#

import sys

PROP_NAME_MAX = 127
PROP_VALUE_MAX = 1023

class PropFile:

    def __init__(self, lines):
        self.props = {}
        self.lines = [s.strip() for s in lines]
        self.__to_dict(self.lines)

    def validate_prop(self, name, value):
        if name == '':
            return False

        if len(name) > PROP_NAME_MAX:
            print('PropFile: WARNING: %s name cannot exceed %d bytes.' %
                           (name, PROP_NAME_MAX))
            return False

        if len(value) > PROP_VALUE_MAX:
            print('PropFile: WARNING: %s value cannot exceed %d bytes.' %
                           (name, PROP_VALUE_MAX))
            return False

        return True

    def __to_dict(self, lines):
        for line in lines:
            if not line or line.startswith("#"):
                continue
            if "=" in line:
                key, value = line.split("=", 1)
                self.set(key, value)

    def get_all(self):
        return self.props

    def get(self, name):
        if name in self.props:
            return self.props[name]
        return ''

    def set(self, name, value):
        name = name.strip()
        value = value.strip()
        if self.validate_prop(name, value):
            self.props[name] = value

    def delete(self, name):
        self.props.pop(name)

    def write(self, filename):
        with open(filename, 'w') as f:
            for key, value in self.props.items():
                f.write(key + '=' + value + '\n')
            f.write('\n')
