#!/usr/bin/env Rscript

suppressMessages (library(ggplot2))
suppressMessages (library(doBy))

source ("graphs/graph-style.R")

input = "results/figure-4-data-propagation-vs-distance/car-relay-in-cache.txt.bz2"
output = "graphs/pdfs/figure-4-data-propagation-vs-distance.pdf"

data <- read.table (bzfile(input, "r"), header=TRUE)
data$DistanceFromSource = data$NodeId * data$Distance
data$Time = data$Time - 2 # we have 2s offset in the data set

## data = subset(data, select=c("Run", "Density", "Time", "NodeId"))

data.speed = summaryBy (DistanceFromSource + Time ~ Run + Distance, data = data, FUN=max )
data.speed$Speed = data.speed$DistanceFromSource / data.speed$Time


g <- ggplot (subset(data.speed), aes(x=Distance, y=Speed)) +
  geom_boxplot (aes(group = as.factor(Distance)), size=0.3, fill='lightblue', outlier.size=1, outlier.colour="gray40")  +
  scale_y_continuous ("Data propagation speed, km/s", labels=function(x){x / 1000}) +
  scale_x_continuous ("Distance between cars, m") +
  theme_custom ()

if (!file.exists ("graphs/pdfs")) {
  dir.create ("graphs/pdfs")
}

pdf (output, width=5, height=3)
g
x= dev.off ()

## , limits=c(-100, 10500)
