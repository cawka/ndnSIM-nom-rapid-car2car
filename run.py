#!/usr/bin/env python
# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

from subprocess import call
from sys import argv
import os
import subprocess
import workerpool
import multiprocessing

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
        self.simulate ()
        pool.join ()
        self.postprocess ()
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

print "\n\n >>> FINISHED <<< \n\n"
