#
# Copyright (c) 2019-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#


######################################################################################
# This script expects an input file that contains the output of the                  #
# OpenCL Intercept Layer, which needs to be run with DevicePerformanceTiming=1 and   #
# DevicePerformanceTimelineLogging=1 for best effect.                                #
# It generates an output file in .xlsx (Excel) format, which can then be opened      #
# to view the parsed data, as well as a bar chart visualizing the Queued, Submitted, #
# and Execution phases of the OpenCL calls.                                          #
######################################################################################


import argparse
import sys
import re

# Note: a recent version of this is needed for the script to run - it has been confirmed to work with version 2.5.0a1
# An error of the format "'Workbook' object has no attribute 'active'" is an indication of an old version 
import openpyxl


class Xlsx_Result:
    def __init__(self):
        self.normalize = None
        self.type = "bar"
        self.title = "OpenCL Intercept Layer"
        self.type = self.type
        self.style = 3
        self.title = self.title
        self.overlap = 100
        self.grouping = "stacked"
        self.y_axis_title = 'Time (ms)'
        self.x_axis_title = 'OpenCL calls'
        self.x_axis_majorTickMark = 'cross'
        self.width = 38.5
        self.shape = 4
        self.legend_position = 't'
        self.queued_color = 'ltGrey'
        self.submitted_color = 'ltBlue'
        self.execution_color = 'orangeRed'

    def insert_xlsx_entries(self, m, result):
        opencl_call = m.group(1)
        if self.normalize == None:
            self.normalize = float(m.group(2))
        values = [float(m.group(x)) - self.normalize if x == 2 else float(m.group(x)) - float(m.group(x-1)) for x in range(2,6)]
        row = (opencl_call,) + tuple(x/1000000.0 for x in values) # convert values from ms to ns
        result.append(row)

    def insert_xlsx_chart(self, rows, name):
        import openpyxl
        wb = openpyxl.workbook.Workbook()
        ws = wb.active
        ws.append(('OpenCL call', '', 'Queued', 'Submitted', 'Execution'))
        for row in reversed(rows):
            ws.append(row)
        
        chart1 = openpyxl.chart.BarChart()
        data = openpyxl.chart.Reference(ws, min_col=2, min_row=1, max_col=5, max_row=len(rows)+1)
        cats = openpyxl.chart.Reference(ws, min_col=1, min_row=2, max_row=len(rows)+1)
        chart1.add_data(data, titles_from_data=True)
        chart1.set_categories(cats)


        chart1.type = self.type
        chart1.style = self.style
        chart1.title = self.title
        chart1.overlap = self.overlap
        chart1.grouping = self.grouping
        chart1.y_axis.title = self.y_axis_title
        chart1.y_axis.minorGridlines = openpyxl.chart.axis.ChartLines()
        chart1.x_axis.title = self.y_axis_title
        chart1.x_axis.majorTickMark = self.x_axis_majorTickMark
        chart1.height = float(len(rows))
        chart1.width = self.width
        chart1.shape = self.shape
        chart1.legend.position = self.legend_position

        chart1.series[0].graphicalProperties.noFill = True # Ignore first region
        chart1.series[1].graphicalProperties.solidFill = openpyxl.drawing.fill.ColorChoice(prstClr=self.queued_color)
        chart1.series[2].graphicalProperties.solidFill = openpyxl.drawing.fill.ColorChoice(prstClr=self.submitted_color)
        chart1.series[3].graphicalProperties.solidFill = openpyxl.drawing.fill.ColorChoice(prstClr=self.execution_color)

        ws.add_chart(chart1, "H1")
        wb.save(name)

def print_lines_ignored(lines):
    from itertools import groupby
    from operator import itemgetter
    message = "Lines ignored:"
    for k, g in groupby(enumerate(lines), lambda x: x[1]-x[0]):
        line_range = list(map(itemgetter(1), g))
        if len(line_range) == 1:
            message += " [" + str(line_range[0]+1) + "]"
        else:
            message += " [" + str(line_range[0]+1) + "-" + str(line_range[-1]+1) + "]"
    print(message)


# To use the script, run:
#       python parse_output_to_xlsx.py <output_file>
def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("--output", help="The name of the output file without file extension (default: trace)",
                        default='trace')
    parser.add_argument("filepath", help="Location/name of the file to be parsed")
    args = parser.parse_args()
    
    # This depends heavily on the format of the debug outputs - when they change, this part of the script might break
    intercept_re = r"Device\sTimeline\sfor\s(.+)\s\(enqueue\s\d+\)\s=\s(\d+)\sns\s\(queued\),\s(\d+)\sns\s\(submit\),\s(\d+)\sns\s\(start\),\s(\d+)\sns\s\(end\).*"
    
    output_name = args.output + '.xlsx'
    trace_obj = Xlsx_Result()

    result = []
    lines_ignored = []
    with open(args.filepath, 'r') as fp:
        for i, line in enumerate(fp):
            m = re.match(intercept_re, line)
            if m:
                trace_obj.insert_xlsx_entries(m,result)
            else:
                lines_ignored.append(i)

    if len(result) == 0:
        print("ERROR: Nothing in the trace. All lines ignored")
    elif len(lines_ignored) != 0:
        print_lines_ignored(lines_ignored)

    trace_obj.insert_xlsx_chart(result,output_name)

if __name__ == "__main__": main()