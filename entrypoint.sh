#!/bin/bash -l

unset CURRENTLY_BUILDING_DOCKER_IMAGE

########################################
#
# Ensure workflow environment exists
#
########################################

if [ -f /home/docker/env.sh ]; then
  echo Workflow environment imported successfully.
else
  echo Workflow environment not specified.
  echo ===================================================
  echo This image designed to be unpacked in a directory
  echo on the host system. Host/container environment
  echo sharing can only be provided if an env.sh
  echo script is copied to /home/docker prior to running
  echo this container.
  echo; 
  echo If you don\'t care about the host environment, 
  echo unpack this container in an arbitrary and:
  echo touch /home/docker/env.sh before running.
  exit 1
fi

# Import workflow environment
source /home/docker/env.sh


########################################
#
#  Setup Docker image environment
#
########################################

export LD_LIBRARY_PATH=/usr/local/lib:${LD_LIBRARY_PATH}
export PATH=/usr/local/bin:/usr/local/sbin:${PATH}
export PYTHONPATH=/usr/local/lib/python2.7/site-packages:${PYTHONPATH}
export INCLUDE=/usr/local/include:${INCLUDE}
export SHARE=/usr/local/share:${SHARE}

########################################


if [ "$1" '=' 'shell' ] ; then
    if [ -t 0 ] ; then
        exec bash -il
    else
        (
            echo -n "It looks like you're trying to run an intractive shell"
            echo -n " session, but either no psuedo-TTY is allocateed for this"
            echo -n " container's STDIN, or it is closed."
            echo

            echo -n "Make sure you run docker with the --interactive and --tty"
            echo -n " options."
            echo
        ) >&2

        exit 1
    fi
elif [[ $1 == "osu" ]]; then
    #passing all but the first arg to osu_bw
    osu_bw "${@:2:$#}"
    exit "$?"

elif [ "$1" '=' 'orted' ]; then
  echo $@;
  "$@";

elif [ "$1" '=' 'generate_dataframe' ]; then
  echo Generating dataframe using container libs
  /usr/bin/python3 ${WORKFLOW_DIR}/post-run/merge_df.py ${WORKFLOW_DIR};

else
  /home/docker/commands.sh "$@" 
  exit $?
fi
