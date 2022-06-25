#!/usr/bin/env python3
# Convert linear list of rows to formatted markdown table.
# Takes input from stdin (if no file is provided) and prints to stdout

import sys
import fileinput

columns = []
current_col = 0

filein = fileinput.FileInput

if len(sys.argv) >= 3:
    # File from arguments
    filein = fileinput.input(files=(sys.argv[2]))
else:
    # File from stdin
    filein = fileinput.input()

for line in filein:
    if line[0]=='#':
        columns.append([line[2:-1]])
        current_col += 1
    else:
        columns[current_col-1].append(line[:-1])

# Collect info about dimensions
max_rows = 0
row_lengths = []
for col in columns:
    title = col[0]
    row_length = len(title)
    max_rows = max(max_rows, len(col))
    for row in col[1:]:
        row_length = max(row_length, len(row))
    row_lengths.append(row_length)
        
# Write column headers
sys.stdout.write("| ")
i = 0
for col in columns:
    title = col[0]
    sys.stdout.write(title)
    sys.stdout.write(' '*(row_lengths[i] - len(title)))
    sys.stdout.write(" | ")
    i += 1

# Write column header delimiters
sys.stdout.write("\n| ")
i = 0
for col in columns:
    sys.stdout.write("-"*row_lengths[i])
    sys.stdout.write(" | ")
    i += 1

# Write rows
print("")
for row in range(1, max_rows):
    sys.stdout.write("| ")
    i = 0
    for col in range(0, len(columns)):
        length = 0
        if len(columns[col]) > row:
            sys.stdout.write(columns[col][row])
            length = len(columns[col][row])
        sys.stdout.write(" "*(row_lengths[i] - length))
        sys.stdout.write(" | ")
        i += 1
    print("")
        
