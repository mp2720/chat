#!/bin/bash

make build || exit $?

WS="10"

cur_ws=`i3-msg -t get_workspaces | jq -r 'map(select(.focused))[0].name'`

make build

i3-msg "workspace number ${WS}"
build/chat
i3-msg "workspace number ${cur_ws}"
