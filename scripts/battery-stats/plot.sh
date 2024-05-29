#!/bin/bash

# Usage: ./plot.sh <filename.csv>
#
# If no filename is given, last csv file is used.

if [ -z "$1" ]; then
  CSV_FILE=$(ls -t *.csv | head -n 1)
  if [ -z "$CSV_FILE" ]; then
    echo "No CSV files found in the current directory."
    exit 1
  fi
else
  CSV_FILE="$1"
fi

# Generate the gnuplot commands
gnuplot -persist <<-EOF
    set datafile separator ","
    set key autotitle columnheader
    set xlabel 'X-axis'
    set ylabel 'Y-axis'
    plot for [i=2:*] '$CSV_FILE' using 1:i with linespoints
EOF

