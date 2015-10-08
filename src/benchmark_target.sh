#!/bin/sh
OUTPUT_DIRECTORY=benchmark/target
RESULT_FILE=${OUTPUT_DIRECTORY}/result.csv
TARGETS=(native broadwell avx2)
rm -rf ${OUTPUT_DIRECTORY}
mkdir -p ${OUTPUT_DIRECTORY}

for target in ${TARGETS[@]}
do
  echo ${target}
  OUTPUT_FILE=${OUTPUT_DIRECTORY}/${target}.txt
  g++ --version > ${OUTPUT_FILE}
  make -e -j4 clean pgo >> ${OUTPUT_FILE}
  echo ${target},`tail -n 1 ${OUTPUT_FILE} | cut -f3 -d" "` >> ${RESULT_FILE}
done

make clean
