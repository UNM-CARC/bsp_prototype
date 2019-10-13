FROM unmcarc/docker_base:spack
WORKDIR /build/
RUN spack create --skip-editor -n sprng \
    && rm /opt/spack/var/spack/repos/builtin/packages/sprng/package.py
COPY sprng.py /opt/spack/var/spack/repos/builtin/packages/sprng/package.py
RUN spack env activate wheeler \
    && spack add sprng \
    && spack concretize \
    && spack install \
    && spack clean -a
#RUN module load gcc/4.8.5/openmpi/3.1.4 && ./configure --with-mpi --with-fortran=no --prefix=/usr && make install
#COPY build_sprng.sh /root/sprng5/build_sprng.sh
#RUN ["chmod", "+x", "/root/sprng5/build_sprng.sh"]
#RUN /root/sprng5/build_sprng.sh
WORKDIR /home/docker/
COPY Makefile bsp_prototype.c gsl-sprng.h /home/docker/
RUN spack env activate wheeler && spack env view enable /usr/local && make bsp_prototype
COPY commands.sh /home/docker/commands.sh
RUN ["chmod", "+x", "/home/docker/commands.sh"]
CMD ["/home/docker/commands.sh"] 
