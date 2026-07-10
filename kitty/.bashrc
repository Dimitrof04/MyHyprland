#
# ~/.bashrc
#

# If not running interactively, don't do anything
[[ $- != *i* ]] && return

alias ls='ls --color=auto'
alias grep='grep --color=auto'
fastfetch
PS1="╭─ \w\n╰─ \u ~\$ "
