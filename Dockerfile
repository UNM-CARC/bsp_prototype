FROM unmcarc/docker_base:latest
COPY . /opt/bsp_prototype
WORKDIR /opt/bsp_prototype/sprng5
RUN /usr/bin/zsh ./installSprng
WORKDIR /opt/bsp_prototype
RUN make
#RUN ls -R /opt/rh/rh-python36
RUN ["chmod", "+x", "/opt/bsp_prototype/commands.sh"]
ENTRYPOINT ["/opt/bsp_prototype/commands.sh"] 
