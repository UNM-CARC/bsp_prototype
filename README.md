# Prototype Data Collection

## data\_gen.py

**data_gen.py**, implements a simple algorithm for running the application originally written by Oscar
Mondragon.

## gather.sh

Shell script which moves/appends results data from /tmp/results on each node back to the file system; after
data is generated and written locally to a directory in the local file system.

## simple.pbs

Submits a batch job for 4 nodes, (default), to run data\_gen.py, which then in turn runs gather.sh after
completion of multiple app\_gen runs using mpi.

## sprng5

### If SPRNG 5 is not already installed on the local system, you may install it by the following command:

cd sprng5 && source installSprng

### To remove local installation:

cd sprng5 && source removeSprng

## perf\_cdse

Contains the latest openspeedshop databases suffixed by .openss and coresponding csv files generated
by the bsp program itself, (These show up in the same order in the directory; i.e. 0.openss corresponds to 
the first csv appearing in the directory).

To pull data from a given openss database run: "./extract\_data.sh <OUTPUTDIR> ...-n.openss"

Currently there already exists a file under output names "0", still in the process of developing a better 
way to name experiments from the openspeedshop database. (This is the CSV ouput from the above command.

Sahba is adding a stencil operation to the code