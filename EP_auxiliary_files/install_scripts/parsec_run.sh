#!/bin/bash
cd parsec-3.0
tar -xvzf ../parsec-3.0-input-sim.tar.gz
source env.sh
parsecmgmt -a info

echo "Building all the benchmarks"
for benchmark in blackscholes ferret fluidanimate raytrace x264 freqmine bodytrack facesim wips dedup canneal water_nsquared fmm water_spacial barnes volrend cholesky 
do
	parsecmgmt -a build -p $benchmark -n 32
done

echo "Running the benchmarks with the input set"
for input in test simdev simsmall simmedium simlarge native
do
	parsecmgmt -a run -p all -i $input -n 32
done
