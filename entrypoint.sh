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

elif [[ $1 == 'rabbit-server' ]]; then
  rabbitmq-server
  exit "$?"
elif [[ $1 == 'rabbit-client' ]]; then
  python /home/docker/rabit_functions.py "${@:1:$#}"
  exit "$?"
elif [ "$1" '=' 'orted' ]; then
  "$@"; 
else
  /home/docker/commands.sh "$@" 
  exit $?
fi

