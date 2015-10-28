#!/bin/sh
OUTPUT_DIRECTORY=benchmark/kpp_padding
RESULT_FILE=${OUTPUT_DIRECTORY}/result.csv
PATH_BASE=${PATH}
rm -rf ${OUTPUT_DIRECTORY}
mkdir -p ${OUTPUT_DIRECTORY}
for kpp_padding0 in 0 1 2 3 4 5 6 7
do 
  for kpp_padding1 in 0 1 2 3 4 5 6 7
  do 
    echo ${kpp_padding0} ${kpp_padding1}
    OUTPUT_FILE=${OUTPUT_DIRECTORY}/${kpp_padding0}-${kpp_padding1}.txt
    g++ --version > ${OUTPUT_FILE}
    make KPP_PADDING0=${kpp_padding0} KPP_PADDING1=${kpp_padding1} -j4 clean pgo
    ./tanuki-clang benchmark_elapsed_for_depth_n >> ${OUTPUT_FILE}
    echo ${kpp_padding0},${kpp_padding1},`tail -n 1 ${OUTPUT_FILE} | cut -f5 -d" "` >> ${RESULT_FILE}
  done
done

make clean
