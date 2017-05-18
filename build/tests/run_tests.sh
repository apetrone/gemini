#!/bin/bash

BIN_PATH=../bin/debug_x86_64

execute_test()
{
	echo "execute ${1}"
	${BIN_PATH}/$*
	result=$?
	if [ $result != 0 ]; then
		echo "\"${1}\" FAILED with return code = ${result}"
		exit 1
	fi
}

execute_test test_core
execute_test test_platform
execute_test test_runtime
execute_test test_render --assets="../assets"