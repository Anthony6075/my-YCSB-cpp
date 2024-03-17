#!/usr/bin/bash

cd $(dirname $0)

TIMES=3
ALL_DBS='hashdb leveldb rocksdb'
ALL_WORKLOADS='a b c d f'

DATE_TIME=$(date '+%y%m%d_%H%M%S')
mkdir -p ./a_learning/"$DATE_TIME"
TMP_LOG_FILE=./a_learning/"$DATE_TIME"/ycsb-tmp.log
RUN_LOG_FILE=./a_learning/"$DATE_TIME"/run.log
RESULT_FILE=./a_learning/"$DATE_TIME"/result

echo "$DATE_TIME"

touch "$RUN_LOG_FILE" "$RESULT_FILE"

for workload in $ALL_WORKLOADS ; do
    for db in $ALL_DBS ; do
        echo "executing workload$workload $db"

        load_throughput=0
        run_throughput=0

        for (( i=0 ; i<$TIMES ; i++ )) ; do
            ./run.sh $db $workload > $TMP_LOG_FILE

            tmp=$(grep 'Load throughput(ops/sec): ' $TMP_LOG_FILE)
            load_tp=${tmp#'Load throughput(ops/sec): '}

            tmp=$(grep 'Run throughput(ops/sec): ' $TMP_LOG_FILE)
            run_tp=${tmp#'Run throughput(ops/sec): '}

            if [[ -z "$run_tp" ]] ; then
                run_tp=0
            fi

            load_throughput=$(python -c "print($load_throughput + $load_tp)")
            run_throughput=$(python -c "print($run_throughput + $run_tp)")

            echo "$ ./run.sh $db $workload" >> $RUN_LOG_FILE
            cat $TMP_LOG_FILE >> $RUN_LOG_FILE
            echo >> $RUN_LOG_FILE
            echo >> $RUN_LOG_FILE

            rm -f $TMP_LOG_FILE
        done

        load_throughput=$(python -c "print(round($load_throughput / $TIMES / 1000))")
        run_throughput=$(python -c "print(round($run_throughput / $TIMES / 1000))")

        echo "$workload    $db    $load_throughput    $run_throughput" >> "$RESULT_FILE"

    done
done

python3 to_chart.py "$RESULT_FILE"
