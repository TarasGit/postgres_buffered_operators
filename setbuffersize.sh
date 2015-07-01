#!/bin/bash
if [ $1 -le 0 ]
  then 
	echo "Buffer size should be >0"
fi
echo "int mybuffer_size  = $1;" > src/backend/access/mybuffer/mybuffer.c
make all -j4
make install
