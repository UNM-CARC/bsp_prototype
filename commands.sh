#!/bin/bash
mkdir -p /tmp/results/
/root/bsp_prototype "$@"
cp /tmp/results/* /results/
