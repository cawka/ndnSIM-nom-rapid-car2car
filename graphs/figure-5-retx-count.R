#!/usr/bin/env Rscript

suppressMessages (library(ggplot2))
suppressMessages (library(doBy))

source ("graphs/graph-style.R")

input1 = "results/car-relay-tx.txt.bz2"
input2 = "results/car-relay-in-cache.txt.bz2"
output = "graphs/pdfs/figure-5-retx-count.pdf"

data <- read.table (bzfile(input1, "r"), header=TRUE)
data$NodeId = as.factor (data$NodeId)
data$Run = as.factor(data$Run)

data = subset(data, Distance %in% c(10, 50, 90, 130, 170))
data$Distance = as.factor(data$Distance)

data.summary = summaryBy (Time ~ NodeId + Run + Distance, data=data, FUN=c(function(x){length(x)-1}))
names(data.summary) = c("NodeId", "Run", "Distance", "Count")

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

g <- ggplot (hist.ci, aes(x=Count, y=NumberOfCars)) +
  geom_bar (aes(fill=Distance), colour=I("black"), size=0.2, stat="identity", binwidth=1, width=0.8, position="dodge") +
  ## geom_errorbar (aes(ymin = NumberOfCars-Interval, ymax=NumberOfCars+Interval, group=Distance), width=1, position="dodge") +
  ## geom_text (aes(x=Count, y=NumberOfCars, label=paste(sep="", round(NumberOfCars,1), "%")), size=I(3), vjust=-1, stat="identity") +
  scale_y_continuous ("Fraction of all cars, %") + #, limits=c(0,65)) +
  scale_x_discrete ("Number of transmissions") +
  ## scale_fill_brewer ("Distance between\ncars, m\n(percent of cars\nreceived data)", palette="RdYlBu", breaks=recv.ci$Distance, labels = recv.ci$Labels) + #start=0.4, end=1,
  theme_custom () +
  opts (legend.position = c(1,1),
        legend.justification=c(1,1),
        legend.text.align=0,
        legend.direction = "vertical",
        legend.key.height = unit(1.0, "lines"))

pdf (args$output, width=5, height=3)
g
x = dev.off ()
