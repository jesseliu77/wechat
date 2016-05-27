ipcs -m | awk '{if($2~/^[0-9]+$/)print $2}'  > out
cat out | while read shid
do
  ipcrm -m $shid
done
