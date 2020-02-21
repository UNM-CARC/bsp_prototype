echo ======= BEGIN: $(hostname) ompi_launch.sh=======

echo Beginning ompi_launch.sh
echo lets see what the command looks like first:
echo $@
export CONTAINER_IMAGE_DIR=$1
source /etc/profile; 

module load openssl-1.1.1b-gcc-4.8.5-obdqvnl; 
module load singularity-3.2.1-gcc-4.8.5-ulix7vo; 

singularity run -B /etc/hostname -B ${CONTAINER_IMAGE_DIR} ${CONTAINER_IMAGE_DIR}/bsp_prototype orted "${@:2:$#}"

echo ======= END: $(hostname) ompi_launch.sh=======
