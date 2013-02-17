#!/usr/bin/env python
# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

from subprocess import call
from sys import argv
import os
import subprocess
import workerpool
import multiprocessing
import argparse

######################################################################
######################################################################
######################################################################

parser = argparse.ArgumentParser(description='Simulation runner')
parser.add_argument('scenarios', metavar='scenario', type=str, nargs='*',
                    help='Scenario to run')

parser.add_argument('-l', '--list', dest="list", action='store_true', default=False,
                    help='Get list of available scenarios')

parser.add_argument('-s', '--simulate', dest="simulate", action='store_true', default=False,
                    help='Run simulation and postprocessing (false by default)')

parser.add_argument('-g', '--no-graph', dest="graph", action='store_false', default=True,
                    help='Do not build a graph for the scenario (builds a graph by default)')

args = parser.parse_args()

if not args.list and len(args.scenarios)==0:
    print "ERROR: at least one scenario need to be specified"
    parser.print_help()
    exit (1)

if args.list:
    print "Available scenarios: "
else:
    if args.simulate:
        print "Simulating the following scenarios: " + ",".join (args.scenarios)

    if args.graph:
        print "Building graphs for the following scenarios: " + ",".join (args.scenarios)

######################################################################
######################################################################
######################################################################

class SimulationJob (workerpool.Job):
    "Job to simulate things"
    def __init__ (self, cmdline):
        self.cmdline = cmdline
    def run (self):
        print (" ".join (self.cmdline))
        subprocess.call (self.cmdline)

pool = workerpool.WorkerPool(size = multiprocessing.cpu_count())

class Processor:
    def run (self):
        if args.list:
            print "    " + self.name
            return

        if "all" not in args.scenarios and self.name not in args.scenarios:
            return

        if args.list:
            pass
        else:
            if args.simulate:
                self.simulate ()
                pool.join ()
                self.postprocess ()
            if args.graph:
                self.graph ()

class CarRelay (Processor):
    def __init__ (self, name, extra=[], runs = range(1,11), distances = range (10, 180, 40)):
        self.name = name
        self.extra = extra
        self.runs = runs
        self.distances = distances

    def simulate (self):
        for distance in self.distances:
            for run in self.runs:
                cmdline = ["./build/car-relay",
                           "--run=%d" % run,
                           "--distance=%d" % distance,
                           ] + self.extra

                job = SimulationJob (cmdline)
                pool.put (job)

    def postprocess (self):
        try:
            os.mkdir ("results/%s" % self.name)
        except:
            pass

        prefix_out = "results/%s/car-relay" % self.name;
        prefix_in = "results/car-relay";
        subtypes = ["jump-distance", "distance", "in-cache", "tx"]
        for subtype in subtypes:
            needHeader = True
            f_out = open ("%s-%s.txt" % (prefix_out, subtype), "w")
            for distance in self.distances:
                for run in self.runs:
                    f_in = open ("%s-%d-%d-%s.txt" % (prefix_in, run, distance, subtype), "r")
                    firstline = f_in.readline ()

                    if needHeader:
                        f_out.write ("Run\tDistance\t%s" % firstline)
                        needHeader = False

                    for line in f_in:
                        f_out.write ("%d\t%d\t%s" % (run, distance, line))

                    f_in.close ()
                    os.remove ("%s-%d-%d-%s.txt" % (prefix_in, run, distance, subtype))
            f_out.close ()
            subprocess.call ("bzip2 -f \"%s-%s.txt\"" % (prefix_out, subtype), shell=True)

    def graph (self):
        subprocess.call ("./graphs/%s.R" % self.name, shell=True)

try:
    # Simulation, processing, and graph building for Figure 3
    fig3 = CarRelay (name="figure-3-data-propagation-vs-time", extra = ["--fixedDistance=10000"], runs = range(1,11), distances = range (10, 180, 40))
    fig3.run ()

    # Simulation, processing, and graph building for Figure 4
    fig4 = CarRelay (name="figure-4-data-propagation-vs-distance", distances = range (10, 160, 5), runs = range(1,11))
    fig4.run ()

    # Simulation, processing, and graph building for Figure 5
    fig5 = CarRelay (name="figure-5-retx-count", runs = range(1,11), distances = range (10, 180, 40))
    fig5.run ()

finally:
    pool.join ()
    pool.shutdown ()
