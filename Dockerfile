FROM unmcarc/docker_base:spack
#RUN yum -y install wget gsl-devel libgfortran gmp-devel rh-python36 rh-python36-numpy zsh openssl-devel perf autoconf build-essential ca-certificates coreutils curl environment-modules git python unzip vim
#RUN yum -y group install "Development Tools"
#RUN ln -s /usr/lib64/openmpi/bin/* /usr/bin/
# mkdir -p /opt/bsp_prototype
#RUN wget -P /opt/bsp_prototype/ https://github.com/UNM-CARC/bsp_prototype/archive/a0a3e9b5f2c31f5cbccce6c1e1b618ea98152d62.zip
#RUN unzip /opt/bsp_prototype/a0a3e9b5f2c31f5cbccce6c1e1b618ea98152d62.zip
COPY sprng5 /root
WORKDIR /root/sprng5
COPY build_sprng.sh .
RUN build_sprng.sh 
WORKDIR /root
COPY Makefile bsp_prototype.c gsl-sprng.h /root
RUN make
COPY commands.sh /root/commands.sh
RUN ["chmod", "+x", "/root/commands.sh"]
CMD ["/root/commands.sh"] 
