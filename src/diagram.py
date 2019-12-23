#
# Copyright (c) 2019 Greg Padiasek
# Distributed under the terms of the the 3-Clause BSD License.
# See the accompanying file LICENSE or the copy at 
# https://opensource.org/licenses/BSD-3-Clause
#

from dateutil.parser import parse
import pylab as pl
import math
import argparse

def load_log(log_file):
  with open(log_file, "r") as log:
    log_data = log.readlines()

  log = []
  for line in log_data:
    word = line.strip().split()
    if len(word) < 6: continue
    date_time = "%s %s" % (word[0], word[1][:-1])
    fitness = word[-1]
    try:
      d = parse(date_time)
      f = float(fitness)
      if math.isnan(f): f = 0
    except:
      continue
    log.append([d,f])

  return log

def draw_log(args, log):
  # datetime in the first log record
  start = log[0][0]
  
  # calculate elapsed time in hours and accuracy in %
  for l in log:
    duration = l[0] - start
    duration_in_s = duration.total_seconds()
    duration_in_h = duration_in_s / 3600
    l[0] = duration_in_h
    l[1] *= 100

  # draw time-accuracy dependency    
  t = [l[0] for l in log] 
  s = [l[1] for l in log]
  pl.plot(t, s)

  # add albels and save
  pl.xlabel('Time (h)')
  pl.ylabel('Accuracy (%)')
  pl.title(args.title)
  pl.grid(True)
  pl.savefig(args.outfile)

# parse input parameters
parser = argparse.ArgumentParser(description='Creates Log Diagrams.')
parser.add_argument("-t", "--title", default="EAGLE Training Log",
                    help='Name of the training dataset.')
parser.add_argument("-l", "--logfile", default="master.log",
                    help='Master training log file.')
parser.add_argument("-o", "--outfile", default="master.png",
                    help='Output file for the training diagram.')
args = parser.parse_args()

# create diagram from training log
draw_log(args, load_log(args.logfile))
