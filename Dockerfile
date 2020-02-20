# By default, we build from a base with the same docker tag as the tag for
# this build, passed in as a build_arg, with the latest (generic) 
# docker base image used by default. However, builds that want to explicitly
# set the base image used for building can control this by setting the 
# BASE_IMAGE variable instead.

ARG DOCKER_TAG=carc-wheeler
ARG BASE_IMAGE=qwofford/docker_ldms:${DOCKER_TAG}
FROM ${BASE_IMAGE}
# Because we use spack and a cleaned environment, the run commands here
# need to be login shells to get the appropriate spack initialiation.
SHELL ["/bin/bash", "-l", "-c"]
RUN spack env activate -d /home/docker

# All commands will run relative to this WORKDIR
WORKDIR /home/docker

# We force our current prototype of the sprng package into the spack
# repo on this container to try it out for now. Once it's added to an
# actual spack repository (e.g. the CARC one), we'll just pull it from 
# there instead.
RUN spack create --skip-editor -n sprng \
    && rm /opt/spack/var/spack/repos/builtin/packages/sprng/package.py
COPY sprng.py /opt/spack/var/spack/repos/builtin/packages/sprng/package.py

# Add the new packages we want to the environment and regenerate its
# view in /usr/local
RUN spack add sprng gsl \
    && spack concretize \
    && spack install \
    && spack env view regenerate \
    && spack clean -a

# Copy bsp prototype files
COPY Makefile bsp_prototype.c gsl-sprng.h /home/docker/
RUN spack add sprng gsl openblas osu-micro-benchmarks \
    && spack concretize \
    && spack install \
    && spack env view regenerate \
    && spack clean -a

# Ask Sahba
RUN yum install -y rabbitmq-server.noarch python3 \
    && pip3 install pika --upgrade \ 
    && echo "[{rabbit, [{loopback_users, []}]}]." \
      > /etc/rabbitmq/rabbitmq.config

# Copy workload execution script
COPY commands.sh /home/docker/commands.sh
RUN make bsp_prototype

# Copy rabbit scripts
COPY rabit_functions.py /home/docker/rabit_functions.py

# Copy environment scripts
COPY entrypoint.sh /home/docker

# Copy ompi launch script
COPY ompi_launch.sh /home/docker


RUN ["chmod", "+x", "/home/docker/commands.sh"]
CMD ["/home/docker/commands.sh"] 
