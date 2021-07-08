#!/bin/bash

set -e
# cross-compile tools dependence
# sudo apt install gcc-aarch64-linux-gnu
# sudo apt install g++-aarch64-linux-gnu

# settings
current_dir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
function print_help() {
    echo "Usage:
    -b  build and install to [output]
    -c  clean all [build] & [output]
    -n  cross-compile option: linux/x86_64,  n = native
example:
    ./build.sh -ncb    // clean & local-native compile & install
    "
}

function create_dir() {
        mkdir -p $1
}

function get_cpu_num() {
        return `cat /proc/cpuinfo | grep processor | wc -l`
}

# 用于前置下载依赖项，一般将项目需要用到的三方库，放在ftp上使用integration.py进行下载
function download_deps() {
        $current_dir/integration.py -${TYPE_CHAR}R . 
}

function upload() {
        echo $1
        if [ "$1" == "loc" ]; then
                cd output
                if [ ! -d "./output" ]; then
                        ln -s ../output output
                fi
                cp ../integration.py integration.py
                cp ../scripts/version_depend_loc.json version_depend.json
                ./integration.py -c u -$TYPE_CHAR
        elif [ "$1" == "thd" ]; then
                cd inner-depend/3rdParty
                if [ ! -d "./output" ]; then
                        ln -s ../3rdParty output
                fi
                cp $current_dir/integration.py integration.py
                cp $current_dir/scripts/version_depend_thd.json version_depend.json
                ./integration.py -c u -$TYPE_CHAR
        else
                echo "unknown option: "$1
        fi
}

function set_debug() {
        export CMAKE_BUILD_TYPE="-DCMAKE_BUILD_TYPE=DEBUG"
}

function native_x86() {
        #source /opt/ros/kinetic/setup.bash
        export CMAKE_TOOLCHAIN=""
        export CMAKE_OPTIONS=$CMAKE_TOOLCHAIN
        export BUILD_TYPE=linux-x86_64
        export TYPE_CHAR=$TYPE_CHAR'n'
}

function set_release() {
        export CMAKE_BUILD_TYPE="-DCMAKE_BUILD_TYPE=MinSizeRel"
}

function  build_clean() {
        rm -rf $current_dir/build/$BUILD_TYPE
        rm -rf $current_dir/opt/$BUILD_TYPE/{bin,lib}
        rm -rf $current_dir/output/$BUILD_TYPE
}

function build_all() {
        create_dir $current_dir/build/$BUILD_TYPE
        cd $current_dir/build/$BUILD_TYPE ; cmake ../../code $CMAKE_OPTIONS $CMAKE_BUILD_TYPE -DBUILD_TYPE=$BUILD_TYPE ;make $JOBS
        if [ $BUILD_TYPE == "qnx-aarch64" ]; then
        cd -
        ./cmake/cmaketool
        fi
}

function gen_proto() {
        protoc=$current_dir/inner-depend/3rdParty/linux-x86_64/protobuf/bin/protoc
        protofile=`ls $current_dir/src/proto_data/proto/*.proto`
        input=" -I=$current_dir/src/proto_data/proto "
        output="--cpp_out=$current_dir/src/proto_data/data"
        $protoc $input $protofile $output
}

# 拷贝输出和依赖项到output
function install_all() {
        mkdir -p $current_dir/output
        mkdir -p $current_dir/output/$BUILD_TYPE/config
        cp -a $current_dir/opt/config/* $current_dir/output/$BUILD_TYPE/config
        cp -a $current_dir/opt/$BUILD_TYPE/* $current_dir/output/$BUILD_TYPE
        cp -a $current_dir/opt/run.sh $current_dir/output/$BUILD_TYPE
        cp $current_dir/version_depend.json $current_dir/output/$BUILD_TYPE
        mkdir -p $current_dir/output/$BUILD_TYPE/lib/3rdlib

}

if [ $# == 0 ]; then
   print_help
   exit -1
fi

while getopts "dibchgnrPj:" arg
do
        case $arg in
        h)      
                print_help
                exit
                ;;
        b)
                build_all
                ;;
        c)
                build_clean
                ;;
        n)
                native_x86
                ;;
        d)
                download_deps
                exit
                ;;
        i)
                install_all
                ;;
        g)
                set_debug
                ;;
        r)
                set_release
                ;;
        j)
                export JOBS="-j$OPTARG"
                ;;
        P)
                gen_proto
                ;;
        esac
done

