#!/bin/bash
mkdir -p /tmp/results/
/opt/bsp_prototype/scripts/app_gen/app_gen_metrics "$@"
cp /tmp/results/* /results/
