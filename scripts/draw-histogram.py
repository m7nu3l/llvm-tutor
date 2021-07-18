#!/usr/bin/env python3
#coding=utf8

import sys
from pygnuplot import gnuplot

def main():
    g = gnuplot.Gnuplot()
    plot_cmd = """
        set output "%s"
        set term png
        binwidth=5
        set boxwidth binwidth
        bin(x,width)=width*floor(x/width)
        set ylabel "Occurrences" 
        set xlabel "Basic block size buckets"
        plot '%s' using (bin($1,binwidth)):(1.0) notitle smooth freq with boxes
        """ % (
        sys.argv[2],
        sys.argv[1])

    g.cmd(plot_cmd)

# First argument is the data file
# Second argument is the output image.
if __name__ == "__main__":
    main()
