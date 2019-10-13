#!/bin/bash
mkdir -p /tmp/results/
/home/docker/bsp_prototype "$@"
cp /tmp/results/* /results/
