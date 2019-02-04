i=(8 64 256)
j=(1024 4096 16384)
for rs in ${i[@]}; do
    for pg in ${j[@]}; do
       mkdir ${rs}_${pg}
   done 
done
