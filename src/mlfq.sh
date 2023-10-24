make clean
make SCHED=MLFQ
cd benchmarks
make clean
make
./genRecord.sh 
cd ../
