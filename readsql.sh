#!/bin/bash
dbname="mydb"
username="taras"
COUNTER=0
while [ $COUNTER -lt 200 ]; do
	echo "Exec Query Nr: $COUNTER"
	psql $dbname $username < select.sql
	let COUNTER+=1
done
