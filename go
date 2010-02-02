#!/usr/bin/env sh

if [ $# -gt 0 ] ; then
	debug/minivm $1
else
	make -f MakeHello.mk
	debug/minivm Hello
fi

