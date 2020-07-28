# By default, we build from a base with the same docker tag as the tag for
# this build, passed in as a build_arg, with the latest (generic) 
# docker base image used by default. However, builds that want to explicitly
# set the base image used for building can control this by setting the 
# BASE_IMAGE variable instead.

#ARG DOCKER_TAG=latest
ARG DOCKER_TAG=hpcg
ARG BASE_IMAGE=unmcarc/docker_base:${DOCKER_TAG}
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
WORKDIR /home/docker
COPY Makefile bsp_prototype.c gsl-sprng.h commands.sh amqp_producer.[c,h] rabit_functions.py /home/docker/
RUN spack add sprng gsl openblas osu-micro-benchmarks\
    && spack concretize \
    && spack install \
    && spack env view regenerate \
    && spack clean -a

# Install python dependencies for analysis scripts
RUN yum install -y python3 python-pandas
RUN pip3 install pandasql

#--user did not work for pip install!

RUN yum install -y rabbitmq-server.noarch librabbitmq-devel python3\
    && pip3 install pika --upgrade\ 
    && echo "[{rabbit, [{loopback_users, []}]}]." > /etc/rabbitmq/rabbitmq.config
RUN make bsp_prototype
RUN ["chmod", "+x", "/home/docker/commands.sh"]
CMD ["/home/docker/commands.sh"] 
