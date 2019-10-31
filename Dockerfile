# By default, we build from a base with the same docker tag as the tag for
# this build, passed in as a build_arg, with the latest (generic) 
# docker base image used by default. However, builds that want to explicitly
# set the base image used for building can control this by setting the 
# BASE_IMAGE variable instead.

ARG DOCKER_TAG=latest
ARG BASE_IMAGE=unmcarc/docker_base:${DOCKER_TAG}
FROM BASE_IMAGE

# We force our current prototype of the sprng package into the spack
# repo on this container to try it out for now. ONce it's added to an
# actual spack repository (e.g. the CARC one), we'll just pull it from 
# there instead.
RUN spack create --skip-editor -n sprng \
    && rm /opt/spack/var/spack/repos/builtin/packages/sprng/package.py
COPY sprng.py /opt/spack/var/spack/repos/builtin/packages/sprng/package.py

# Add the new packages we want to the environment and regenerate its
# view in /usr/local
WORKDIR /home/docker
RUN spack add sprng gsl \
    && spack concretize \
    && spack install \
    && spack env view regenerate \
    && spack clean -a

# Finally, compile any addtional programs needed locally and set our command
# that will be run on container start.
COPY Makefile bsp_prototype.c gsl-sprng.h /home/docker/
COPY commands.sh /home/docker/commands.sh
RUN make bsp_prototype
RUN ["chmod", "+x", "/home/docker/commands.sh"]
CMD ["/home/docker/commands.sh"] 
