#!/bin/sh
OUTPUT_DIRECTORY=benchmark/cache_size
RESULT_FILE=${OUTPUT_DIRECTORY}/result.csv
PATH_BASE=${PATH}
rm -rf ${OUTPUT_DIRECTORY}
mkdir -p ${OUTPUT_DIRECTORY}
for tt_size in "1024" "2048" "4096" "8192"
do 
  for eval_size in 0x8000000LL 0x10000000LL 0x20000000LL 0x40000000LL
  do 
    if [ ${tt_size} = "8192" ] && [ ${eval_size} = 0x40000000LL ]
    then
      break
    fi
    echo ${tt_size} ${eval_size}
    OUTPUT_FILE=${OUTPUT_DIRECTORY}/${tt_size}-${eval_size}.txt
    make USI_HASH_FOR_BENCHMARK=\"${tt_size}\" EVALUATE_TABLE_SIZE=${eval_size} -j4 clean pgo
    ./tanuki-clang benchmark_elapsed_for_depth_n >> ${OUTPUT_FILE}
    echo ${tt_size},${eval_size},`tail -n 1 ${OUTPUT_FILE} | cut -f5 -d" "` >> ${RESULT_FILE}
  done
done

make clean
