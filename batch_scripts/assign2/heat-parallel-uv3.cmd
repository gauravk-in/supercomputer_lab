#!/bin/bash

#SBATCH -o $HOME/workspace/batch/heat.$JOB_ID.out
#SBATCH -D  /home/cluster/h039v/h039val
#SBATCH -J heat-parallel
#SBATCH --clusters=uv3
#SBATCH --get-user-env
#SBATCH --ntasks=64
#SBATCH --mail-type=end
#SBATCH --mail-user=gaurav.kukreja@tum.de
#SBATCH --export=NONE
#SBATCH --time=08:00:00

source /etc/profile.d/modules.sh

srun_ps /home/cluster/h039v/h039val/workspace/parallel-tars/run-parallel-tests-uv.sh

