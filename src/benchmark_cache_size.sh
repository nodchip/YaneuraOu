#!/bin/sh
OUTPUT_DIRECTORY=benchmark/cache_size
RESULT_FILE=${OUTPUT_DIRECTORY}/result.csv
PATH_BASE=${PATH}
rm -rf ${OUTPUT_DIRECTORY}
mkdir -p ${OUTPUT_DIRECTORY}
for USI_HASH_FOR_BENCHMARK in "1024" "2048" "4096" "8192"
do 
  for EVALUATE_TABLE_SIZE in 0x8000000LL 0x10000000LL 0x20000000LL 0x40000000LL
  do 
    if [ ${USI_HASH_FOR_BENCHMARK} = "8192" ] && [ ${EVALUATE_TABLE_SIZE} = 0x40000000LL ]
    then
      break
    fi

    if [ ${USI_HASH_FOR_BENCHMARK} = "8192" ] && [ ${EVALUATE_TABLE_SIZE} = 0x20000000LL ]
    then
      break
    fi

    if [ ${USI_HASH_FOR_BENCHMARK} = "4096" ] && [ ${EVALUATE_TABLE_SIZE} = 0x40000000LL ]
    then
      break
    fi

    echo ${USI_HASH_FOR_BENCHMARK} ${EVALUATE_TABLE_SIZE}
    OUTPUT_FILE=${OUTPUT_DIRECTORY}/${USI_HASH_FOR_BENCHMARK}-${EVALUATE_TABLE_SIZE}.txt
    make USI_HASH_FOR_BENCHMARK=\\\"${USI_HASH_FOR_BENCHMARK}\\\" EVALUATE_TABLE_SIZE=${EVALUATE_TABLE_SIZE} -j4 clean pgo
    ./tanuki-clang benchmark_elapsed_for_depth_n >> ${OUTPUT_FILE}
    echo ${USI_HASH_FOR_BENCHMARK},${EVALUATE_TABLE_SIZE},`tail -n 1 ${OUTPUT_FILE} | cut -f5 -d" "` >> ${RESULT_FILE}
  done
done

make clean
