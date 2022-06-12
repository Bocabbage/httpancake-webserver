for i in `seq 50`
do
    echo ${i} ":" `./nonBlockingLogTest && wc -l ./NonBlockingLogTest.log`
    
done