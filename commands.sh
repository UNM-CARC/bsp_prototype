#!/bin/bash
scl enable rh-python36 bash
/opt/rh/rh-python36/root/bin/python3.6 /opt/bsp_prototype/data_gen.py
cat /tmp/results/*

