#!/usr/bin/env Rscript

suppressMessages (library(ggplot2))
## suppressMessages (library(doBy))

source ("graphs/graph-style.R")

input = "results/figure-3-data-propagation-vs-time/car-relay-jump-distance.txt.bz2"
output = "graphs/pdfs/figure-3-data-propagation-vs-time.pdf"

data <- read.table (bzfile(input, "r"), header=TRUE)
data$Run = as.factor(data$Run)
data$DistanceFromSource = data$NodeId * data$Distance

data = subset(data, Distance %in% c(10, 50, 90, 130, 170))
data$Distance = as.factor(data$Distance)


g <- ggplot(data, aes(x=Time-2, y=DistanceFromSource, colour=Distance, linetype=Distance)) +
  geom_line (aes(group=paste(Distance, Run, fill=Distance)), show_guide=FALSE, size=0.2) +

  ## stat_summary(aes(group=Distance, colour=Distance), fun.y="mean", geom="line", size = 2) +
  ## geom_line (aes(group=Distance, y=mean(DistanceFromSource/1000))) +
  geom_smooth (aes(fill=Distance), show_guide=TRUE, method="lm", colour='black', n=20, size=0.3) +

  scale_y_continuous ("Distance from source, km", limits=c(-500,10000), labels=function(x){x/1000}) +
  scale_x_continuous ("Time, seconds", limits=c(0,2)) +
  scale_colour_brewer("Distance between cars, m", palette="Dark2", guide = "none") +
  scale_fill_brewer("Distance between cars, m", palette="Dark2") +
  scale_linetype_manual("Distance between cars, m", values=c(1,2,3,4,5,6,7,8)) +
  theme_custom () +
  theme (legend.position = c(0.99,-0.05), legend.justification=c(1.0,0), legend.direction = "horizontal")

if (!file.exists ("graphs/pdfs")) {
  dir.create ("graphs/pdfs")
}

pdf (output, width=5, height=3.5)
g
x = dev.off ()
