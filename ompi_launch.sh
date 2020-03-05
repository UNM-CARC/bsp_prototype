echo ======= BEGIN: $(hostname) ompi_launch.sh=======

echo Beginning ompi_launch.sh
source /etc/profile;

module load tacc-singularity/3.4.2

#singularity run -B /etc/hostname -B ${CONTAINER_IMAGE_DIR} ${CONTAINER_IMAGE_DIR}/bsp_prototype orted $@
singularity run -B /etc/hostname -B ${SCRATCH}/$(whoami) ${SCRATCH}/$(whoami)/bsp_prototype orted $@

echo ======= END: $(hostname) ompi_launch.sh=======
