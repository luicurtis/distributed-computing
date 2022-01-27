#!/bin/bash
#
#SBATCH --cpus-per-task=8
#SBATCH --time=02:00
#SBATCH --mem=1G
#SBATCH --partition=slow

srun /home/cwlui/cmpt431/assignment2/heat_transfer_parallel --nThreads 8 --tSteps 5000 --gSize 1000 --mTemp 1000 --iCX 0.05 --iCY 0.1

