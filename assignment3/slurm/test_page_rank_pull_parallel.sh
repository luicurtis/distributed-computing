#!/bin/bash
#
#SBATCH --cpus-per-task=8
#SBATCH --time=02:00
#SBATCH --mem=1G
#SBATCH --partition=slow
#SBATCH --nodelist=cs-cloud-02


srun python /home/cwlui/cmpt431/assignment3/scripts/page_rank_tester.pyc --execPath=/home/cwlui/cmpt431/assignment3/ --scriptPath=/home/cwlui/cmpt431/assignment3/scripts/page_rank_evaluator.pyc --inputPath=/scratch/input_graphs/