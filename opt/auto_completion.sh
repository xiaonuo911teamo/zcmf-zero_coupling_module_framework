# 功能
# 支持定位程序 run.sh 启动的自动补全. 如输入 ./run.sh -r [Tab][Tab] 会出现可选列表
#
# 使用方法
# source auto_completion.sh
# 
# 可以将这条命令加在 ~/.bashrc 中,启动窗口自动执行

function auto_completion() {
    local cur prev opts 
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    opts="test"

    if [[ $prev == "-r" ]] || [[ $prev == "-nr" ]]; then
        COMPREPLY=( $(compgen -W "${opts}" -- ${cur}) )
    fi
    return 0
}
complete -F auto_completion run.sh
