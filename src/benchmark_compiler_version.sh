#!/bin/sh
OUTPUT_DIRECTORY=benchmark/compiler_version
RESULT_FILE=${OUTPUT_DIRECTORY}/result.csv
PATH_BASE=${PATH}
rm -rf ${OUTPUT_DIRECTORY}
mkdir -p ${OUTPUT_DIRECTORY}
for directory in `ls -1 "/c/Program Files/mingw-w64"`
do
  PATH="/c/Program Files/mingw-w64/${directory}/mingw64/bin":${PATH_BASE}
  echo ${directory}
  OUTPUT_FILE=${OUTPUT_DIRECTORY}/${directory}.txt
  g++ --version > ${OUTPUT_FILE}
  make -e -j4 clean pgo >> ${OUTPUT_FILE}
  echo ${directory},`tail -n 1 ${OUTPUT_FILE} | cut -f3 -d" "` >> ${RESULT_FILE}
done

make clean
