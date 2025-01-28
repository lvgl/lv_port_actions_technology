import os
import re
import argparse

class scan_file:
    def __init__(self, file_path, header_dir):
        if os.path.exists(file_path) != True or os.path.isdir(header_dir) != True:
            raise Exception('init fail')
        self.input_file = file_path
        self.header_dir = header_dir

    def __read_file(self, file_path):
        headers = set()
        with open(file_path, 'r', errors='ignore') as file:
            for line in file:
                match = re.match(r'#include\s*("[^"]*gx_[^"]*"|<gx_[^>]+>)', line)
                ""
                if match:
                    header_name = match.group(1)
                    if header_name.startswith('"'):
                        header_name = header_name.strip('"')
                        headers.add(header_name)
                    else:
                        headers.add(header_name)

        return headers

    def __get_gx_headers(self, path):
        headers = set()
        if os.path.isdir(path):
            for foldername, subfolders, filenames in os.walk(path):
                for filename in filenames:
                    file_path = os.path.join(foldername, filename)
                    headers.update(self.__read_file(file_path))
        else:
            headers = self.__read_file(path)
        return headers

    def get_headfile_name(self, input_file = None, head_list = None):
        if head_list is None:
            head_list = set()

        if input_file is None:
            headers =  self.__get_gx_headers(self.input_file)
        else:
            headers =  self.__get_gx_headers(input_file)

        head_list.update(headers)
        for item in headers:
            item_path = os.path.join(self.header_dir, item).replace('\\', '/')
            if os.path.isfile(item_path) == True:
                self.get_headfile_name(item_path, head_list)

        return head_list

    def delete_files_except_whitelist(self, headers):
        whitelist = set()
        for item in headers:
            if '/' in item:
                whitelist.add(item.split('/')[-1])
            else:
                whitelist.add(item)
        for root, dirs, files in os.walk(self.header_dir):
            for file in files:
                if file not in whitelist:
                    remove_full_path = os.path.join(root, file)
                    print("Deleting",remove_full_path)
                    os.remove(remove_full_path)

parser = argparse.ArgumentParser()
parser.add_argument("-s", "--source", help="scan source file/dir.")
parser.add_argument("-i", "--include", help="include dir.")
args = parser.parse_args()

if args.source == None:
    args.source = os.path.join(os.getcwd(), 'src')

if args.include == None:
    args.include = os.path.join(os.getcwd(), 'include')

print(args.source)
print(args.include)

scan = scan_file(args.source, args.include)
headers = scan.get_headfile_name()
scan.delete_files_except_whitelist(headers)
