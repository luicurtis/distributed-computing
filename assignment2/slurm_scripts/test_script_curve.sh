#!/bin/bash
#
#SBATCH --cpus-per-task=8
#SBATCH --time=02:00
#SBATCH --mem=1G
#SBATCH --partition=fast

srun python /home/cwlui/cmpt431/assignment2/test_scripts/curve_area_tester.pyc --execPath=/home/cwlui/cmpt431/assignment2/curve_area_parallel --scriptPath=/home/cwlui/cmpt431/assignment2/test_scripts/curve_area_evaluator.pyc