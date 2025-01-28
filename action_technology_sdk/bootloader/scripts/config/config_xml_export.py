# -*- coding: utf-8 -*-
import os
import re
import sys
import struct
import collections
import argparse

try:
    import xml.etree.cElementTree as ET
except ImportError:
    import xml.etree.ElementTree as ET

import logger


class XML_Parse(object):
    """
    XML file parser.
    """
    def __init__(self, log):
        self.LOG = log
        self.xml_root       = None

    def open_xml(self, xml_handle):
        if os.path.isfile(xml_handle):
            tree            = ET.ElementTree(file = xml_handle)
            self.xml_root   = tree.getroot()
        elif isinstance(xml_handle, basestring):
            self.xml_root   = ET.fromstring(xml_handle)
        else:
            raise Exception("%r NOT a XML file or XML string" %xml_handle)

    def enumerate_all_subroot(self, root = None):
        """Enumerate all sub item of the root.
        """
        subroot_list = []
        if root is None:
            root = self.xml_root

        for sub_root in root.iter():
            # self.LOG.debug(sub_root.tag, sub_root.attrib)
            subroot_list.append(sub_root)

        return subroot_list

    def enumerate_subroot(self, key, root = None):
        """Enumerate sub item of the root's first level.
        """
        if root is None:
            root = self.xml_root

        return root.findall(key)

    def indent(self, elem, level = 0):
        """Add indent for the xml string
        """
        i = "\n" + level * "    "
        if len(elem):
            if not elem.text or not elem.text.strip():
                elem.text = i + "    "
            if not elem.tail or not elem.tail.strip():
                elem.tail = i
            for elem in elem:
                xml_indent(elem, level + 1)
            if not elem.tail or not elem.tail.strip():
                elem.tail = i
        else:
            if level and (not elem.tail or not elem.tail.strip()):
                elem.tail = i



class ConfigXML(XML_Parse):
    """
    config xml file parser.
    """
    def __init__(self, log):
        super().__init__(log)

        self.cfg_key    = "config"
        self.item_key   = "item"

        self.config_name_key    = "name"
        self.config_id_key      = "cfg_id"
        self.config_size_key    = "size"

        self.item_name_key      = "name"
        self.item_offset_key    = "offs"
        self.item_size_key      = "size"

        self.config_name_p = re.compile(r'^\s*CFG_(\w+)\s*')
        self.item_type_p = re.compile(r'^\s*(\w+)\s+(\w+)[^;]+;', re.M)
        self.cfg_type_size_d = {'cfg_uint8' : 1, 'cfg_uint16' : 2, 'cfg_uint32' : 4}

    def get_config_structure(self, cfg_src_c_file):
        with open(cfg_src_c_file, 'r', encoding = 'utf-8') as f:
            cfg_c_source_str = f.read()

        class_def_p = re.compile(r'^class\s+', re.M)
        position_list = []
        for class_obj in class_def_p.finditer(cfg_c_source_str):
            position_list.append((class_obj.start(), class_obj.end()))

        cfg_class_count = len(position_list)
        self.LOG.debug("Has %d classes" %cfg_class_count)
        position_list.append((len(cfg_c_source_str), 0))
        self.LOG.debug(position_list)

        cfg_structure_dict = {}
        for i in range(cfg_class_count):
            structure_str = cfg_c_source_str[position_list[i][1] : position_list[i + 1][0]]
            cfg_obj = self.config_name_p.match(structure_str)
            if not cfg_obj:
                self.LOG.error("NOT support config pattern %s" %structure_str[:20])
                continue

            config_name = cfg_obj.groups()[0]
            cfg_structure_dict[config_name] = []
            for item_obj in self.item_type_p.finditer(structure_str, pos = cfg_obj.end()):
                info_tuple = item_obj.groups()
                cfg_structure_dict[config_name].append(info_tuple)

        self.LOG.debug(cfg_structure_dict)
        return cfg_structure_dict

    def get_config_key_info(self, cfg_xml_file, cfg_src_c_file):
        self.open_xml(cfg_xml_file)
        cfg_structure_dict = self.get_config_structure(cfg_src_c_file)

        # info pattern: config name, config id, offset, size
        config_info_dict = {}

        cfg_element_list = self.enumerate_subroot(self.cfg_key)
        for cfg_root in cfg_element_list:
            config_name = cfg_root.attrib[self.config_name_key]
            config_id   = int(cfg_root.attrib[self.config_id_key], 0)
            config_size = int(cfg_root.attrib[self.config_size_key], 0)

            item_element_list = self.enumerate_subroot(self.item_key, cfg_root)
            item_name_list = []
            for item_element in item_element_list:
                item_name   = item_element.attrib[self.item_name_key]
                item_offset = int(item_element.attrib[self.item_offset_key], 0)
                item_size   = int(item_element.attrib[self.item_size_key], 0)
                config_info_dict["%s_%s" %(config_name, item_name)] = (config_id, item_offset, item_size)
                item_name_list.append(item_name)

            hide_items = len(cfg_structure_dict[config_name]) - len(item_element_list)
            if hide_items > 0:
                self.LOG.debug("Has %d hide items in %s" %(hide_items, config_name))

                item_info_list = cfg_structure_dict[config_name]
                item_offset = 0
                for item_type, item_name in item_info_list:
                    if item_type not in self.cfg_type_size_d:
                        self.LOG.debug("NOT support type %s for %s" %(item_type, item_name))
                    elif item_name not in item_name_list:
                        item_size = self.cfg_type_size_d[item_type]
                        item_offset = (item_offset + item_size - 1) // item_size * item_size
                        self.LOG.info("Hide item %s offset %d, size %d" %(item_name, item_offset, item_size))
                        config_info_dict["%s_%s" %(config_name, item_name)] = (config_id, item_offset, item_size)
                        item_offset += item_size
                    else:
                        info_tuple = config_info_dict["%s_%s" %(config_name, item_name)]
                        item_offset = info_tuple[1] + info_tuple[2]

        # print(config_info_dict)
        return config_info_dict

    def export_config_item_info(self, cfg_xml_file, cfg_src_c_file, output_file):
        """Build driver header file, include all config item
        """
        head_macro = os.path.basename(output_file).split(".")[0].upper()
        config_info_dict = self.get_config_key_info(cfg_xml_file, cfg_src_c_file)
        config_info_list = [(x, *y) for x, y in config_info_dict.items()]
        config_info_list.sort(key = lambda x : (x[1], x[2]))
        self.LOG.debug(config_info_list)

        with open(output_file, r'w', encoding = 'utf-8') as f_w:
            f_w.write("\n\n#ifndef __%s_H__\n#define __%s_H__\n\n" %(head_macro, head_macro))

            prv_id = -1
            for config_name, config_id, item_offset, item_size in config_info_list:
                key_value = ((config_id & 0xFF) << 24) + ((item_offset & 0xFFF) << 12) + (item_size & 0xFFF)
                if prv_id != config_id:
                    f_w.write("\n")

                prv_id = config_id
                f_w.write("#define CFG_%s %s(0x%08x)    // id:%3d, off:%4d, size:%4d\n"
                    %(config_name, ''.ljust(67 - len(config_name)), key_value, config_id, item_offset, item_size))

            f_w.write("\n\n#endif  // __%s_H__\n\n" %(head_macro))



def main(argv):
    parser = argparse.ArgumentParser(
        description='Export driver config key from config XML file.',
    )

    parser.add_argument('-i',   dest = 'cfg_xml_file',  required = True,                help = 'input config xml files')
    parser.add_argument('-c',   dest = 'cfg_src_file',  required = True,                help = 'input config c source files')
    parser.add_argument('-o',   dest = 'output_file',   default = 'drv_config_head.h',  help = 'output driver config header file')

    args            = parser.parse_args()
    cfg_xml_file    = args.cfg_xml_file
    cfg_src_c_file  = args.cfg_src_file
    output_file     = args.output_file

    log = logger.Logger()
    # log = logger.Logger('1.txt', Flevel = logger.LOGLEVEL['DEBUG'])
    # log = logger.Logger(clevel = logger.LOGLEVEL['DEBUG'])

    if not os.path.isfile(cfg_xml_file):
        log.error("%s NOT exist" %cfg_xml_file)
        sys.exit(1)

    if not os.path.isfile(cfg_src_c_file):
        log.error("%s NOT exist" %cfg_src_c_file)
        sys.exit(1)

    cfg_xml = ConfigXML(log)
    cfg_xml.export_config_item_info(cfg_xml_file, cfg_src_c_file, output_file)


if __name__ == "__main__":
    main(sys.argv[1:])

