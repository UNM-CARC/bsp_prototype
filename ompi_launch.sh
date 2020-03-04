echo ======= BEGIN: $(hostname) ompi_launch.sh=======

echo Beginning ompi_launch.sh
source /etc/profile; 

module load openssl-1.1.1b-gcc-4.8.5-obdqvnl; 
module load singularity-3.2.1-gcc-4.8.5-ulix7vo; 

#singularity run -B /etc/hostname -B ${CONTAINER_IMAGE_DIR} ${CONTAINER_IMAGE_DIR}/bsp_prototype orted $@
singularity run -B /etc/hostname -B /wheeler/scratch/$(whoami) /wheeler/scratch/$(whoami)/bsp_prototype orted $@

echo ======= END: $(hostname) ompi_launch.sh=======
