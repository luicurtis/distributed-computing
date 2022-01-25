#!/bin/bash
#
#SBATCH --cpus-per-task=8
#SBATCH --time=02:00
#SBATCH --mem=1G
#SBATCH --partition=slow

srun python /home/cwlui/cmpt431/assignment2/test_scripts/heat_transfer_tester.pyc --execPath=/home/cwlui/cmpt431/assignment2/heat_transfer_parallel --scriptPath=/home/cwlui/cmpt431/assignment2/test_scripts/heat_transfer_evaluator.pyc