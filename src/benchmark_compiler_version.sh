#!/bin/sh
OUTPUT_DIRECTORY=benchmark
RESULT_FILE=${OUTPUT_DIRECTORY}/result.csv
TARGETS=(native broadwell avx2)
PATH_BASE=${PATH}
rm -rf ${OUTPUT_DIRECTORY}
mkdir -p ${OUTPUT_DIRECTORY}
for directory in `ls -1 "/c/Program Files/mingw-w64"`
do
  PATH="/c/Program Files/mingw-w64/${directory}/mingw64/bin":${PATH_BASE}
  for target in ${TARGETS[@]}
  do
    echo ${directory} ${target}
    OUTPUT_FILE=${OUTPUT_DIRECTORY}/${directory}-${target}.txt
    g++ --version > ${OUTPUT_FILE}
    make -e -j4 clean pgo >> ${OUTPUT_FILE}
    echo ${directory},${target},`tail -n 1 ${OUTPUT_FILE} | cut -f3 -d" "` >> ${RESULT_FILE}
  done
done

make clean
