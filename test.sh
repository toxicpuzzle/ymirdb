#!/bin/bash

# Trigger all your test cases with this script

FILES=$(ls tests/*.in | sed -e 's/\.in$//')

for file in $FILES; do
    echo " "
    echo "===== Executing test case: ${file} ====="
    echo " "
    #include the diff command to differentitate test case output with actual output
    valgrind ./ymirdb < $file.in >> output.txt #$| diff - $file.out 

done;