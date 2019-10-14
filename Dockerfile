FROM unmcarc/docker_base:master

# We force our current prototype of the sprng package into the spack
# repo on this container to try it out for now. ONce it's added to an
# actual spack repository (e.g. the CARC one), we'll just pull it from 
# there instead.
RUN spack create --skip-editor -n sprng \
    && rm /opt/spack/var/spack/repos/builtin/packages/sprng/package.py
COPY sprng.py /opt/spack/var/spack/repos/builtin/packages/sprng/package.py

# Add the new packages we want to the environment and regenerate its
# view in /usr/local
RUN spack env activate wheeler \
    && spack add sprng \
    && spack concretize \
    && spack install \
    && spack env view regenerate /usr/local \
    && spack clean -a

# Finally, compile any addtional programs needed locally and set our command
# that will be run on container start.
WORKDIR /home/docker/
COPY Makefile bsp_prototype.c gsl-sprng.h /home/docker/
RUN spack env activate wheeler  \
    && make bsp_prototype
COPY commands.sh /home/docker/commands.sh
RUN ["chmod", "+x", "/home/docker/commands.sh"]
CMD ["/home/docker/commands.sh"] 
