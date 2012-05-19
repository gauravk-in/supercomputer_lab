#!/bin/sh

#if [ "$1"=="" ]
#then
#	echo "Invalid usage: Specify ice or mpp or uv."
#	exit 1
#fi

MACHINE=$1

#path to assignment 3 source code
PATH_SOURCE="/home/cluster/h039v/h039val/assign3/supercomputer/mpi-assign3"
OUTPUT_PATH="/home/cluster/h039v/h039val/assign3/batch_result/"$MACHINE"-"$(date +%T)

if [ "$MACHINE" == "ice" ] 
then
	EXEC_CMD="mpirun -np"
	num_of_proc=("4" "8" "16" "32" "64")
elif [ "$MACHINE" == "mpp" ] 
then
	EXEC_CMD="mpiexec -n"
	num_of_proc=("4" "8" "16" "32" "64")
elif [ "$MACHINE" == "uv" ]
then
	EXEC_CMD="mpirun -np"
	num_of_proc=("4" "8" "16" "32" "64" "128")
fi

echo $EXEC_CMD

cd $PATH_SOURCE
rm -rf $OUTPUT_PATH
mkdir $OUTPUT_PATH

#make clean
#make > $OUTPUT_PATH"/make-result-mpp.out"

for i in ${num_of_proc[@]}
do
$EXEC_CMD $i ./reduction > $OUTPUT_PATH"/reduction_"$i".out"
done

for i in ${num_of_proc[@]} 
do
$EXEC_CMD $i ./ping-pong $OUTPUT_PATH"/ping_pong_"$i".out"
done

for i in ${num_of_proc[@]}
do
$EXEC_CMD $i ./ping-pong-bandwidth $OUTPUT_PATH"/bandwidth_"$i".out"
done

exit
