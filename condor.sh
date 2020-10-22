#!/bin/bash
# the 1 is the file name and 2 is the file line
line=$(($2 + 1))

cd /home/criley38/filter-marta-data
echo ${line}

FILE_LINE=$(sed -n "${line}p" ${1})
IFS=',' read -ra array <<< "$FILE_LINE"
for i in "${array[@]}"; do
    echo "$i"
done

INPUT_NAME="${array[0]}"
echo ${INPUT_NAME}

./build/Project ${INPUT_NAME}
