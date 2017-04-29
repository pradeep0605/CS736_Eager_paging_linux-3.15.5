#!/bin/bash

#setup things for tracing in the kernel
PATH=/sys/kernel/debug/tracing/
echo 0 > $PATH/tracing_on
echo 'kmem:mm_page_alloc' > $PATH/set_event
echo $$ > $PATH/set_ftrace_pid
echo '' > $PATH/trace
echo 1 > $PATH/tracing_on
/usr/bin/strace -f $* 2> out
echo 0 > $PATH/tracing_on
