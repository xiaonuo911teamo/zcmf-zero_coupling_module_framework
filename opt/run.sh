#! /bin/bash
ulimit -c unlimited

current_dir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

function print_help() {
    echo "./run.sh -r [cmdline]"
}

function native_x86() {
	export BUILD_TYPE=linux-x86_64
}

function run() {
    export LD_LIBRARY_PATH=$current_dir/$BUILD_TYPE/lib:$current_dir/$BUILD_TYPE/lib/3rdlib:$LD_LIBRARY_PATH
    if [ $1 == "test" ]; then
        $current_dir/$BUILD_TYPE/bin/iv_task server_log server_proc
    fi
}

if [ $# == 0 ]; then
   print_help
   exit -1
fi

while getopts "nr:" arg
do
    case $arg in
        n)
            native_x86
            ;;
        r)
            run $OPTARG
            ;;
    esac
done

wait
