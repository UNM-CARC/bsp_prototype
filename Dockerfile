FROM unmcarc/docker_base:latest
#RUN yum -y install wget gsl-devel libgfortran gmp-devel rh-python36 rh-python36-numpy zsh openssl-devel perf autoconf build-essential ca-certificates coreutils curl environment-modules git python unzip vim
#RUN yum -y group install "Development Tools"
#RUN ln -s /usr/lib64/openmpi/bin/* /usr/bin/
COPY . /opt/bsp_prototype
# mkdir -p /opt/bsp_prototype
#RUN wget -P /opt/bsp_prototype/ https://github.com/UNM-CARC/bsp_prototype/archive/a0a3e9b5f2c31f5cbccce6c1e1b618ea98152d62.zip
#RUN unzip /opt/bsp_prototype/a0a3e9b5f2c31f5cbccce6c1e1b618ea98152d62.zip
WORKDIR bsp_prototype/sprng5
RUN /usr/bin/zsh ./installSprng
WORKDIR /opt/bsp_prototype
RUN make
#COPY commands.sh /scripts/commands.sh
RUN ["chmod", "+x", "/opt/bsp_prototype/commands.sh"]
ENTRYPOINT ["/opt/bsp_prototype/commands.sh"] 
