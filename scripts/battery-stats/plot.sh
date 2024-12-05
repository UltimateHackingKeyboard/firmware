#!/bin/bash

# Find the last modified CSV file in the current directory if no parameter is given
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
    set y2label 'Y2-axis'
    set grid
    set grid ytics lt 1 lw 1 lc rgb "#bbbbbb"
    set ytics 100
    set y2tics
    set mxtics 5
    set mytics 5
    set terminal wxt size 1200,800

    # Set the y range for the first y axis
    set yrange [2000:4300]

    # Set up the second y axis with range
    set y2range [0:5]

    # Determine the number of columns in the CSV file
    n = system(sprintf("awk -F',' 'NR==1{print NF}' %s", '$CSV_FILE'))

    # Plot the first and second columns on the y2 axis (second y axis)
    plot '$CSV_FILE' using 1:2 title columnheader(2) with linespoints axis x1y2, \
         '$CSV_FILE' using 1:3 title columnheader(3) with linespoints axis x1y2, \
         for [i=4:n] '$CSV_FILE' using 1:i title columnheader(i) with linespoints axis x1y1
EOF
