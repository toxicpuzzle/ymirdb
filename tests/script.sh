FILES=$(ls *.in | sed -e 's/\.in$//')
for f in $FILES; do 
    sed -i -e '$a\' "${f}.out"
    # mv -- "${f}.outs" "${f}.out"
done;