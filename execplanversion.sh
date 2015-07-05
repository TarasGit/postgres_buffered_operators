#!/bin/bash
if [ $1 -l 0 ]
  then 
	echo "0=buffer with qual-flag "
	echo "1=full buffer"
	echo "2=original version of PostgreSQL"
fi
echo "int mybuffer_size  = $1;" > src/backend/access/mybuffer/myexecplan.c
make all -j4
make install
