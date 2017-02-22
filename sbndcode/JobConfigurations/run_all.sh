#!/bin/bash



for files in `ls *.fcl`
do

echo "lar -c $files -n 40 -o ${files}_gen.root"
echo "lar -c standard_g4_sbnd.fcl -n 40 ${files}_gen.root -o ${files}_gen_g4.root" 
echo "lar -c standard_detsim_sbnd.fcl -n 40 ${files}_gen_g4.root -o ${files}_gen_g4_detsim.root" 

lar -c $files -n 40 -o ${files}_gen.root
lar -c standard_g4_sbnd.fcl -n 40 ${files}_gen.root -o ${files}_gen_g4.root 
lar -c standard_detsim_sbnd.fcl -n 40 ${files}_gen_g4.root -o ${files}_gen_g4_detsim.root 

done 