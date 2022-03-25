#!/bin/bash

# Trigger all your test cases with this script

FILES=$(ls tests/*.in | sed -e 's/\.in$//')
F=0 # No spaces in assignment
C=0
for file in $FILES; do
    
    if (test -f "$file.in") && (test -f "$file.out"); then
        printf "\n"
        printf "Executing test case: ${file}%5s\n"
        printf "\n"
        if [[ $(./ymirdb < ${file}.in | diff -w - ${file}.out) ]]; then
            echo "     Test case failed"
            echo " "
            ./ymirdb < ${file}.in | diff -w - ${file}.out
        else
            echo "     Test case passed"
            C=$((C + 1))
        fi
        F=$((F + 1))
    fi
    #include the diff command to differentitate test case output with actual output
done;

printf "\nPassed ${C}/${F} test cases\n"