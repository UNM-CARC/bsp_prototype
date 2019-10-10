FROM unmcarc/docker_base:spack
#RUN yum -y install wget gsl-devel libgfortran gmp-devel rh-python36 rh-python36-numpy zsh openssl-devel perf autoconf build-essential ca-certificates coreutils curl environment-modules git python unzip vim
#RUN yum -y group install "Development Tools"
#RUN ln -s /usr/lib64/openmpi/bin/* /usr/bin/
# mkdir -p /opt/bsp_prototype
#RUN wget -P /opt/bsp_prototype/ https://github.com/UNM-CARC/bsp_prototype/archive/a0a3e9b5f2c31f5cbccce6c1e1b618ea98152d62.zip
#RUN unzip /opt/bsp_prototype/a0a3e9b5f2c31f5cbccce6c1e1b618ea98152d62.zip
COPY sprng5/ /home/docker/sprng5/
WORKDIR /home/docker/sprng5
RUN module load openmpi-3.1.4-gcc-4.8.5-chobv7u && ./configure --with-mpi --with-fortran=no --prefix=/usr && make install
COPY build_sprng.sh /home/docker/sprng5/build_sprng.sh
RUN ["chmod", "+x", "/home/docker/sprng5/build_sprng.sh"]
RUN /home/docker/sprng5/build_sprng.sh
WORKDIR /home/docker
COPY Makefile bsp_prototype.c gsl-sprng.h /home/docker/
RUN module load openmpi-3.1.4-gcc-4.8.5-chobv7u && make bsp_prototype
COPY commands.sh /home/docker/commands.sh
RUN ["chmod", "+x", "/home/docker/commands.sh"]
CMD ["/home/docker/commands.sh"] 
