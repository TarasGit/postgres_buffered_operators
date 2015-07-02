#!/bin/bash
COUNTER=0
echo "Create Directory for pg-Log extraction[max=100]"
if [ ! -d results ]; then
	echo "Create Dir results" 
	mkdir results
fi

while [ $COUNTER -lt 100 ]; do
if [ ! -d results/$COUNTER ]; then
	echo "Create Dir results/$COUNTER";
	mkdir results/$COUNTER
	break
fi
let COUNTER+=1
done

grep -o '[0-9]\+\.[0-9]\{3\}' pglog.log | nl > results/$COUNTER/res.log

cd results/$COUNTER/

gnuplot <<- EOF
set xlabel "Anzahl"
set ylabel "ms"
set term png
set output "res.png"
plot "res.log"
EOF

	

	
