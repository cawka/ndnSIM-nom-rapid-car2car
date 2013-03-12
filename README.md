Prerequisites
=============

Custom version of NS-3 and **specified version** of ndnSIM needs to be installed.

The code should also work with the latest version of ndnSIM, but it is not guaranteed.

    git clone git@github.com:cawka/ns-3-dev-ndnSIM.git -b ns-3.16-ndnSIM ns-3
    git clone git@github.com:NDN-Routing/ndnSIM.git -b v0.2.8 ns-3/src/ndnSIM

    cd ns-3
    ./waf configure
    ./waf install

For more information how to install NS-3 and ndnSIM, please refer to http://ndnsim.net website.

Compiling
=========

``./waf configure``

or (configure in debug mode with logging enabled)

``./waf configure --debug``

If you have installed NS-3 in a non-standard location, you may need to set up ``PKG_CONFIG_PATH`` variable.
For example, if NS-3 is installed in /usr/local/, then the following command should be used to
configure scenario

``PKG_CONFIG_PATH=/usr/local/lib/pkgconfig ./waf configure``

or

``PKG_CONFIG_PATH=/usr/local/lib/pkgconfig ./waf configure --debug``

Running
=======

Normally, you can run scenarios either directly

``./build/<scenario_name>``

or using waf

``./waf --run <scenario_name>``

If NS-3 is installed in a non-standard location, on some platforms (e.g., Linux) you need to specify ``LD_LIBRARY_PATH`` variable:

``LD_LIBRARY_PATH=/usr/local/lib ./build/<scenario_name>``

or

``LD_LIBRARY_PATH=/usr/local/lib ./waf --run <scenario_name>``


When running using ./waf, it is possible to run scenario with visualizer:

``./waf --run <scenario_name> --vis``


Available simulations
=====================

For more detail refer to L. Wang, A. Afanasyev, R. Kuntz, R. Vuyyuru, R. Wakikawa, and L. Zhang, "Rapid Traffic Information Dissemination Using Named Data," in Proceedings of the 1st ACM workshop on Emerging Name-Oriented Mobile Networking Design - Architecture, Algorithms, and Applications (NoM 12), Hilton Head Island, South Carolina, June 2012, pp. 7–12. (http://dx.doi.org/10.1145/2248361.2248365, http://lasr.cs.ucla.edu/afanasyev/data/files/Wang/nom.pdf)

## Data propagation over time (Figure 3)

Simulation scenario:

    scenarios/car-relay.cc

To automatically run 10 runs of simulations with different parameters for the figure and build a graph for the simulation:

    ./waf
    ./run.py -s figure-3-data-propagation-vs-time

To rebuild the graph without rerunning the simulation:

    ./run.py figure-3-data-propagation-vs-time

User ``./run.py -h`` for other options.

**Note that provided scripts rely on R (http://www.r-project.org/) with ggplot2 module to be installed.**

## Data propagation speed vs distance between cars (Figure 4)

Simulation scenario:

    scenarios/car-relay.cc

To automatically run 10 runs of simulations with different parameters for the figure:

    ./waf
    ./run.py -s figure-4-data-propagation-vs-distance

To rebuild the graph without rerunning the simulation:

    ./run.py figure-4-data-propagation-vs-distance

## Count of data transmissions (Figure 5)

Simulation scenario:

    scenarios/car-relay.cc

To automatically run 10 runs of simulations with different parameters for the figure:

    ./waf
    ./run.py -s figure-5-retx-count

To rebuild the graph without rerunning the simulation:

    ./run.py figure-5-retx-count
