#!/usr/bin/env Rscript

suppressMessages (library(ggplot2))
suppressMessages (library(doBy))

source ("graphs/graph-style.R")

input1 = "results/figure-5-retx-count/car-relay-tx.txt.bz2"
input2 = "results/figure-5-retx-count/car-relay-in-cache.txt.bz2"
output = "graphs/pdfs/figure-5-retx-count.pdf"

data <- read.table (bzfile(input1, "r"), header=TRUE)
data$NodeId = as.factor (data$NodeId)
data$Run = as.factor(data$Run)

data = subset(data, Distance %in% c(10, 50, 90, 130, 170))
data$Distance = as.factor(data$Distance)

data.summary = summaryBy (Time ~ NodeId + Run + Distance, data=data, FUN=c(function(x){length(x)}))
names(data.summary) = c("NodeId", "Run", "Distance", "Count")

data.summary = subset (data.summary, Count <= 8)

data.hist = summaryBy (Count ~ Count + Run + Distance, data = data.summary, FUN=c(length))
names(data.hist) = c("Count", "Run", "Distance", "NumberOfCars")


data.hist$Count = as.factor (data.hist$Count)

conf.interval = 0.98
hist.ci = summaryBy (NumberOfCars ~ Count + Distance, data=data.hist,
  FUN=c(
    function(x) {
      mean (x) / 10 # /1000 cars * 100%
      },
    function(x) {
      ciMult <- qt(conf.interval, length(x)-1)
      return (ciMult * sd(x)/sqrt(length(x)) / 10) # /1000 cars * 100%
    }))
names(hist.ci) = c("Count", "Distance", "NumberOfCars", "Interval")

data.recv <- read.table (bzfile(input2, "r"), header=TRUE)
recv = summaryBy (NodeId ~ Run + Distance, data.recv, FUN=length)
names(recv)[3] = "Count"

recv.ci = summaryBy (Count ~ Distance, recv,
  FUN=c(
    function(x) {
      mean (x) / 10
    },
    function(x) {
      ciMult <- qt(conf.interval, length(x)-1)
      return (ciMult * sd(x)/length(x) / 10) # /1000 cars * 100%
    }))
names(recv.ci) = c("Distance", "CachedMean", "CachedInterval")

x= paste (sep="", recv.ci$Distance, " (", round(recv.ci$CachedMean,1), "%)")
recv.ci$Labels = factor(x, levels=x, ordered=TRUE)

## print (hist.ci)

data.totals = summaryBy (NumberOfCars + Interval ~ Distance, data = hist.ci, FUN=sum)

hist.ci.updated = rbind (
  data.frame (Count = c (0,0,0,0,0),
              Distance = recv.ci$Distance,
              NumberOfCars = recv.ci$CachedMean - data.totals$NumberOfCars.sum,
              Interval = recv.ci$CachedInterval + data.totals$Interval.sum),
  hist.ci)
hist.ci.updated$NumberOfCars[hist.ci.updated$NumberOfCars < 0] = 0

hist.ci.updated$Distance = factor( as.double(hist.ci.updated$Distance) )


g <- ggplot (hist.ci.updated, aes(x=Count, y=NumberOfCars)) +
  geom_bar (aes(fill=Distance), colour=I("black"), size=0.2, stat="identity", binwidth=1, width=0.8, position="dodge") +

  scale_y_continuous ("Fraction of all cars, %") +
  scale_x_discrete ("Number of transmissions") +
  scale_fill_brewer ("Distance between\ncars, m\n(percent of cars\nreceived data)", palette="RdYlBu", breaks=recv.ci$Distance, labels = recv.ci$Labels) +
  theme_custom () +
  theme (legend.position = c(1,1),
        legend.justification=c(1,1),
        legend.text.align=0,
        legend.direction = "vertical",
        legend.key.height = unit(1.0, "lines"))

pdf (output, width=5, height=3)
g
x = dev.off ()
