#!/bin/bash
path=$1
rm -rf /home/dew/test/Dec25dewArchival/$path.csv
rm -rf /home/dew/test/Dec25dewArchival/$path.csv.end
#echo '0,块数量,时间,速度,1,块数量,时间,速度,2,块数量,时间,速度,3,块数量,时间,速度,4,块数量,时间,速度,5,块数量,时间,速度6,块数量,时间,速度,7,块数量,时间,速度,8,块数量,时间,速度'>$path.csv
#echo 'num0,time,speed,num1,time,speed,num2,time,speed,num3,time,speed,num4,time,speed,num5,time,speed,num6,time,speed,num7,time,speed,num8,time,speed'>$path.csv
echo 'arriv,num0,time,speed,arriv,num1,time,speed,arriv,num2,time,speed,arriv,num3,time,speed,arriv,num4,time,speed,arriv,num5,time,speed,arriv,num6,time,speed,arriv,num7,time,speed,arriv,num8,time,speed'>$path.csv
for i in $(seq 0 30)
do
val1=$((2+$i*9))
val2=$((10+$i*9))
sed -n ''$val1','$val2'p' $path|sed 's/[[:alpha:]]//g' |sed 's/_//g' |sed 's/^.//g'|sed 's/\s//g'|sort -k1 >$path.csv.temp
awk 'BEGIN{FS=","};{print $1","$2","$6","$7","$8","$9}' $path.csv.temp >> $path.csv.end
done
for j in $(seq 0 30)
do
val3=$((1+$j*9))
val4=$((9+$j*9))
sed -n ''$val3','$val4'p' $path.csv.end|awk 'BEGIN{FS=","};{if((double)strtonum($4)>0)printf("%d,%d,%d,%.2lf,",$3,$4,$5,(double)strtonum($4)/((double)strtonum($5)+0.01));else printf("%d,%d,%d,%.2lf,",$3,$4,$5,0)}'|sed 's/$/\n/g' >>$path.csv
#sed -n ''$val3','$val4'p' $path.csv.end|awk 'BEGIN{FS=","};{if((double)strtonum($4)>0)printf("%d,%d,%.2lf,",$3,$4,(double)strtonum($3)/((double)strtonum($4)+0.01));else printf("%d,%d,%.2lf,",$3,$4,-1)}'|sed 's/$/\n/g' >>$path.csv
#sed -n ''$val3','$val4'p' $path.csv|awk 'BEGIN{FS=","};{if((double)strtonum($4)>0)printf("%d,%d,%.2lf,",$3,$4,(double)strtonum($3));else printf("%d,%d,%.2lf,",$3,$4,-1)}'|sed 's/$/\n/g' >>$path.csv.end

done
