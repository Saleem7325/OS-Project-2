make clean
make SCHED=PSJF
cd benchmarks
make clean
make
./genRecord.sh 
cd ../
