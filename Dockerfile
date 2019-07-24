FROM unmcarc/docker_base:latest
COPY . /opt/bsp_prototype
WORKDIR /opt/bsp_prototype/sprng5
RUN /usr/bin/zsh ./installSprng
WORKDIR /opt/bsp_prototype
#RUN module add mpi/openmpi3-x86_64 && make
RUN make
RUN ["chmod", "+x", "/opt/bsp_prototype/commands.sh"]
ENTRYPOINT ["/opt/bsp_prototype/commands.sh"] 
