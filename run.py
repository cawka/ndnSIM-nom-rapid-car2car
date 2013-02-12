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

class CarRelay:
    def __init__ (self, runs, distances):
        self.runs = runs
        self.distances = distances

    def simulate (self):
        for distance in self.distances:
            for run in self.runs:
                cmdline = ["./build/car-relay",
                           "--run=%d" % run,
                           "--distance=%d" % distance,
                           "--fixedDistance=10000",
                           ]
                job = SimulationJob (cmdline)
                pool.put (job)

    def postprocess (self):
        import os

        prefix = "results/car-relay";

        subtypes = ["jump-distance", "distance", "in-cache", "tx"]

        for subtype in subtypes:
            needHeader = True
            f_out = open ("%s-%s.txt" % (prefix, subtype), "w")
            for distance in self.distances:
                for run in self.runs:
                    f_in = open ("%s-%d-%d-%s.txt" % (prefix, run, distance, subtype), "r")
                    firstline = f_in.readline ()

                    if needHeader:
                        f_out.write ("Run\tDistance\t%s" % firstline)
                        needHeader = False

                    for line in f_in:
                        f_out.write ("%d\t%d\t%s" % (run, distance, line))

                    f_in.close ()
                    os.remove ("%s-%d-%d-%s.txt" % (prefix, run, distance, subtype))
            f_out.close ()
            subprocess.call ("bzip2 -f \"%s-%s.txt\"" % (prefix, subtype), shell=True)

try:
    car_relay = CarRelay (runs = range(1,11),
                          distances = range (10, 180, 5))
    car_relay.simulate ()
    pool.join ()
    car_relay.postprocess ()



finally:
    pool.join ()
    pool.shutdown ()

print "\n\n >>> FINISHED <<< \n\n"
