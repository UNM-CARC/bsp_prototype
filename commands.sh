scl enable rh-python36 bash
module load mpi/openmpi3-x86_64
mkdir -p /tmp/results/
echo "Running with commands $@"
/opt/bsp_prototype/scripts/app_gen/app_gen_metrics "$@"
cp /tmp/results/* /results/
