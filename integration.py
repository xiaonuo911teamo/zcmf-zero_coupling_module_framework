#!/usr/bin/python
########################################################
#
# Copyright 2019 Baidu Inc. All Rights Reserved.
#
########################################################
"""
integration
"""
import os
import sys
import getopt
import json
import tarfile
import socket
import urllib

# global setting
g_root_path = os.path.split(os.path.realpath(__file__))[0]
g_download_path = g_root_path + "/inner-depend"
g_library_path = g_root_path + "/inner-depend/3rdParty"
g_json_file_name = "version_depend.json"
g_tool_file = os.path.split(os.path.realpath(__file__))[1]
g_supported_modules = ["zcmf", "zeromq", "protobuf"]
g_modules = {}

def error_message(msg):
    """Print the error message in red."""
    print("\033[1;31m[ERROR] %s\033[0m" % msg)
def info_message(msg):
    """Print the info message in green."""
    print("\033[1;32m[INFO] %s\033[0m" % msg)
    # -1:failed 0:success 1:no such file

class Module(object):
    """Module"""

    def __init__(self, parent_module_name, parent_version_id, module_name, version_id, link):
        """set module data"""
        self.parent_module_name = parent_module_name
        self.parent_version_id = parent_version_id
        self.module_name = module_name
        self.version_id = version_id
        self.link = link

class Downloader(object):
    """Downloader"""

    def __init__(self):
        """init"""
        self.__root_path = ""
        self.__download_path = ""
        self.__download_platforms = []

    def download(self):
        """download depends"""
        ret = 0
        # parse arg
        if self.__parse_arg() != 0:
            ret = -1
        else:
            self.__print_env()
        # get need modules
        parent_module_name = [""]
        parent_version_id = [""]
        need_modules = {}
        if ret == 0:
            if self.__valid_version_depend(self.__root_path + "/" + g_json_file_name) != 0:
                ret = -1
            else:
                self.__get_need_modules(parent_module_name, parent_version_id, need_modules)
        # make downlaod path
        if ret == 0:
            if self.__make_dir(self.__download_path) != 0:
                ret = -1
        # download modules
        if ret == 0:
            for platform in self.__download_platforms:
                info_message("**** download " + platform + " ****")
                self.downloaded_map = {}
                self.downloading_statck = []
                self.downloading_statck.append(parent_module_name[0])
                has_error = 0
                for key in need_modules:
                    if self.__download_module(key, need_modules[key], platform) != 0:
                        has_error = 1
                        break
                if has_error == 1:
                    ret = -1
                    break
        # last log
        if ret == 0:
            info_message("==== success ====")
        else:
            info_message("==== failed ====")
        return ret

    def __get_need_modules(self, parent_module_name, parent_version_id, need_modules):
        parent_module_name[0] = self.version_depend_obj["module"]
        parent_version_id[0] = "v_" + self.version_depend_obj["version"]
        module_depends = self.version_depend_obj["depends"]
        for i in module_depends:
            need_modules[i["name"]] = Module(parent_module_name, parent_version_id, i["name"], i["version_id"], i["link"])

    def __valid_module_name(self, module_name):
        ret = 0
        if module_name not in g_supported_modules:
            ret = -1
        if ret != 0:
            supported_modules = ""
            for i in g_supported_modules:
                supported_modules += " " + i
            error_message("module name error: " + module_name + " (supported modules:" + supported_modules + ")")
        return ret

    def __valid_version(self, version):
        ret = 0
        number_list = version.split(".")
        if len(number_list) == 4:
            for i in number_list:
                if not i.isdigit():
                    ret = -1
                    break
        else:
            ret = -1
        if ret != 0:
            error_message("version format error: " + version + " (eg: 1.0.0.0)")
        return ret

    def __valid_version_depend(self, json_file):
        ret = 0
        info_message("read " + json_file + " ...")
        json_str = [""]
        if self.__read_json_file(json_file, json_str) != 0:
            ret = -1
        else:
            try:
                json_obj = json.loads(json_str[0])
                if self.__valid_module_name(json_obj["module"]) != 0:
                    ret = -1
                if self.__valid_version(json_obj["version"]) != 0:
                    ret = -1
                module_depends = json_obj["depends"]
                for i in module_depends:
                    if self.__valid_module_name(i["name"]) != 0:
                        ret = -1
                if ret == 0:
                    self.version_depend_obj = json_obj
            except:
                ret = -1
                error_message("version_depend format error: " + json_file)
        return ret

    def __make_dir(self, path):
        ret = 0
        try:
            if not os.path.exists(path):
                os.makedirs(path)
        except:
            ret = -1
            error_message("make directory failed: " + path)
        return ret

    def __remove_path(self, path):
        ret = 0
        try:
            if os.path.exists(path):
                info_message("remove " + path + " ...")
                shutil.rmtree(path)
        except:
            ret = -1
            error_message("remove " + path + " failed")
        return ret

    def __remove_file(self, file_path):
        if os.path.isfile(file_path):
            info_message("remove " + file_path + " ...")
            os.remove(file_path)


    def __get_module_depends(self, json_file, module_depends):
        ret = 0
        info_message("read " + json_file + " ...")
        # to read json file
        json_str = [""]
        if self.__read_json_file(json_file, json_str) != 0:
            ret = -1
        # parse json data
        if ret == 0:
            try:
                json_obj = json.loads(json_str[0])
                module_depends += json_obj["depends"]
            except:
                ret = -1
                error_message("json format error: " + json_file)
        return ret

    def __download_module_depends(self, json_file, parent_module_name, parent_version_id, platform):
        ret = 0
        module_depends = []
        if self.__get_module_depends(json_file, module_depends) != 0:
            ret = -1
        if ret == 0:
            for module in module_depends:
                if self.__download_module(module["name"], \
                    module["version_id"], parent_module_name, parent_version_id, platform) != 0:
                    ret = -1
                    break
        return ret

    def download_file(self, local_file, remote_file):
        """download file"""
        ret = 0
        urllib.urlretrieve(remote_file, local_file)
        return ret

    def __download_platform(self, module_name, sub_module, platform):
        ret = 0
        file_path = g_download_path + "/" + module_name + "-" + sub_module.version_id + ".tar.gz"
        print "downloading " + module_name
        if self.download_file(file_path, sub_module.link) != 0:
            ret = -1
        print "done. saved to " + file_path
        target_path = g_library_path + "/" + platform
        print "tar file ..."
        if self.__tar_file(file_path, target_path) != 0:
            ret = -1
        print "done. tared to " + target_path
        return ret

    def __download_module(self, module_name, sub_module, platform):
        ret = 0
        info_message("download " + module_name + ": " + sub_module.link + " " + platform + " ...")
        # check is downloaded
        is_download = 0
        if ret == 0:
            if module_name in self.downloaded_map:
                is_download = 1
                info_message(module_name + "-" + sub_module.version_id + " already downloaded")
            else:
                self.downloading_statck.append(module_name)
        # download
        if ret == 0 and is_download == 0:
            if self.__download_platform(module_name, sub_module, platform) != 0:
                print "__download_platform failed. "
                ret = -1
            else:
                json_file = g_library_path + "/" + platform + "/" + module_name + "/" + g_json_file_name
                ret = self.__download_module_depends(json_file, module_name, sub_module, platform) # recursion
            # update downloading statck
            if ret == 0:
                self.downloaded_map[module_name] = module_name
                self.downloading_statck.pop()
        return ret

    def __tar_file(self, file_path, target_path):
        ret = 0
        info_message("tar " + file_path + " ...")
        try:
            tar = tarfile.open(file_path, "r:gz")
            try:
                tar.extractall(target_path)
            except:
                ret = -1
                error_message("tar " + file_path + " failed")
            finally:
                tar.close()
        except:
            ret = -1
            error_message("open " + file_path + " failed")
        return ret

    def __read_json_file(self, json_file, json_str):
        ret = 0
        try:
            file = open(json_file, "r")
            try:
                json_str[0] = file.read()
            except:
                ret = -1
                error_message("read " + json_file + " failed")
            finally:
                file.close()
        except:
            ret = -1
            error_message("open " + json_file + " failed")
        return ret

    def __print_env(self):
        info_message("================ env bengin ================")
        info_message("root path: " + self.__root_path)
        info_message("download path: " + self.__download_path)
        tmp_str = ",".join(self.__download_platforms)
        info_message("download platforms: " + tmp_str)
        info_message("================ env end    ================")

    def __parse_arg(self):
        ret = 0
        try:
            opts, args = getopt.getopt(sys.argv[1:], "R:npqlvahA")
            for op, value in opts:
                if op == "-R":
                    self.__root_path = g_root_path
                    self.__download_path = g_download_path
                elif op == "-n":
                    self.__download_platforms.append("linux-x86_64")
                elif op == "-p":
                    self.__download_platforms.append("linux-aarch64")
                elif op == "-q":
                    self.__download_platforms.append("qnx-aarch64")
                elif op == "-l":
                    self.__download_platforms.append("linaro-aarch64")
                elif op == "-v":
                    self.__download_platforms.append("qnx-x86_64")
                elif op == "-a":
                    self.__download_platforms.append("poky-aarch32")
                elif op == "-A":
                    self.__download_platforms.append("poky-aarch64")
                elif op == "-h":
                    ret = -1
                    break
        except getopt.GetoptError as error:
            ret = -1
            error_message(error)
        # check arguments
        if ret == 0:
            if self.__root_path == "":
                ret = -1
                error_message("argument error: root path is empty")
            if len(self.__download_platforms) == 0:
                ret = -1
                error_message("argument error: download platforms is empty")
        if ret != 0:
            self.__usage()
        return ret

    def __usage(self):
        """print usage"""
        info_message("================ usage bengin ================")
        info_message("-R: root path")
        info_message("-n: platform option, linux-x86_64,  n = native")
        info_message("-p: platform option, linux-aarch64, p = px2")
        info_message("-q: platform option, qnx-aarch64,   q = qnx")
        info_message("-l: platform option, linaro-aarch64,   l = linaro")
        info_message("-v: platform option, qnx-x86_64,    v = vmware")
        info_message("-a: platform option, poky-aarch32,  a = poky")
        info_message("-h: help")
        info_message("example:")
        info_message("    ./download-depend.py -R . -n")
        info_message("    ./download-depend.py -R . -npqv")
        info_message("================ usage end    ================")


# entry
inst = Downloader()
inst.download()

