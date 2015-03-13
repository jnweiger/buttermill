#!/usr/bin/python
# Copyright (c) 2010, Ciaran Farrell <cfarrell1980@gmail.com>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are permitted
# provided that the following conditions are met:
#
#    * Redistributions of source code must retain the above copyright notice, this list of conditions
#      and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions
#      and the following disclaimer in the documentation and/or other materials provided with the distribution.
#    * Ciaran Farrell, the buttermill project and its contributors may not be used to endorse or
#      promote products derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
# TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# This script parses the output of the buttermill wind logger
from datetime import datetime
import sys
import os
import re

# these config settings should be taken from your anemometer handbook
ROTOR_1 = 1.144
ROTOR_2 = 1.144
ROTOR_3 = 1.144

class ButtermillParser:

  def __init__(self,input_file):
    try:
      self.fd = open(input_file,'r')
    except IOError,e:
      raise IOError,"Could not open %s for parsing: %s"%(input_file,str(e))
    else:
      pass
    self.data_container = [] # all reading dicts are stored here
    self.badlines = [] # store the lines that didn't parse too
    self.gaga = []

  def parse(self):
    # TODO: secs=1285858049 w1=294 w2=365 w3=482 temp=+26 light=005
    # Some of the strings written to the sd card are corrupted - there are some
    # corruptions which occur often enough in the same place that we can subsitute
    # the corrupted character. For example secs= is normally correctly written but
    # there are numerous examples of sec3= being written
    pattern = re.compile(r'sec[3s]=(?P<secs>[\d]+)\s.*?w1=\s*(?P<w1>[\d]+)\s.*?w2=\s*(?P<w2>[\d]+)\s.*?w3=\s*(?P<w3>[\d]+)\s.*?temp=\s*(?P<t>[\+\-]?[\.\d]+)\s.*?light=\s*(?P<l>[\d]+)')
    i = 0 # keep count of where we are (needed to avoid certain lines)
    for line in self.fd:
      m = pattern.match(line)
      if not m:
        self.badlines.append(line)
        continue
      else:
        try:
          p = self.data_container[i-1]
        except IndexError:
          have_predecessor = False
        else:
          have_predecessor = True
        container = {}
        container['raw_secs'] = int(m.groupdict()['secs'])
        container['tstamp'] = datetime.fromtimestamp(container['raw_secs'])
        if container['tstamp'].year != 2010:
          self.gaga.append(line)
        container['raw_w1'] =   int(m.groupdict()['w1'])
        if have_predecessor:
          old_secs = self.data_container[i-1]['raw_secs']
          secs_passed = container['raw_secs'] - old_secs
          container['w1_ticks_per_sec'] = (container['raw_w1']/secs_passed)*1.0 # pulses per sec (as float)
          container['w1_avg_ms'] = container['w1_ticks_per_sec']*ROTOR_1
        container['raw_w2'] =   int(m.groupdict()['w2'])
        if have_predecessor:
          old_secs = self.data_container[i-1]['raw_secs']
          secs_passed = container['raw_secs'] - old_secs
          container['w2_ticks_per_sec'] = (container['raw_w2']/secs_passed)*1.0 # pulses per sec (as float)
          container['w2_avg_ms'] = container['w2_ticks_per_sec']*ROTOR_2
        container['raw_w3'] =   int(m.groupdict()['w3'])
        if have_predecessor:
          old_secs = self.data_container[i-1]['raw_secs']
          secs_passed = container['raw_secs'] - old_secs
          container['w3_ticks_per_sec'] = (container['raw_w3']/secs_passed)*1.0 # pulses per sec (as float)
          container['w3_avg_ms'] = container['w3_ticks_per_sec']*ROTOR_3
        t =    m.groupdict()['t']
        if t.startswith('+'):
          t = float(t.strip('+'))
        else:
          t = -1.0*(float(t.strip('-')))
        container['temp'] = t
        container['light'] =  int(m.groupdict()['l'])
        self.data_container.append(container)
      i+=1

  def __calcmean__(self,entries):
    if len(entries) == 0:
      raise StandardError,'Division by zero because of empty input list'
    return float(sum(entries)/len(entries))

  def mean(self,period='daily'):
    yearly,monthly,weekly,daily,hourly,minutely = {},{},{},{},{},{}
    period = period.lower()
    if period not in ['minutely','hourly','daily','weekly','monthly','yearly']:
      raise StandardError, '%s is not a recognised mean period'%period
    if len(self.data_container) == 0:
      raise StandardError, 'There is no data available in the data container'
    if period == 'yearly':
      for entry in self.data_container[1:]:
        t = entry
        if yearly.has_key(t['tstamp'].year):
          yearly[t['tstamp'].year]['r1'].append(t['w1_avg_ms'])
          yearly[t['tstamp'].year]['r2'].append(t['w2_avg_ms'])
          yearly[t['tstamp'].year]['r3'].append(t['w3_avg_ms'])
          yearly[t['tstamp'].year]['t'].append(t['temp'])
          yearly[t['tstamp'].year]['l'].append(t['light'])
        else:
          yearly[t['tstamp'].year] = {'r1':[t['w1_avg_ms']],'r2':[t['w2_avg_ms']],'r3':[t['w3_avg_ms']],\
                            't':[t['temp']],'l':[t['light']]}
      means = {}
      for year in yearly.keys():
        means[year] = {}
        yearly[year]['r1_mean'] = self.__calcmean__(yearly[year]['r1'])
        means[year]['r1'] = yearly[year]['r1_mean']
        yearly[year]['r2_mean'] = self.__calcmean__(yearly[year]['r2'])
        means[year]['r2'] = yearly[year]['r2_mean']
        yearly[year]['r3_mean'] = self.__calcmean__(yearly[year]['r3'])
        means[year]['r3'] = yearly[year]['r3_mean']
        yearly[year]['t_mean'] = self.__calcmean__(yearly[year]['t'])
        means[year]['t'] = yearly[year]['t_mean']
        yearly[year]['l_mean'] = self.__calcmean__(yearly[year]['l'])
        means[year]['l'] = yearly[year]['l_mean']
      return means

    if period == 'monthly':
      for entry in self.data_container[1:]:
        t = entry
        month_key = t['tstamp'].strftime("%Y-%m")
        if monthly.has_key(month_key):
          monthly[month_key]['r1'].append(t['w1_avg_ms'])
          monthly[month_key]['r2'].append(t['w2_avg_ms'])
          monthly[month_key]['r3'].append(t['w3_avg_ms'])
          monthly[month_key]['t'].append(t['temp'])
          monthly[month_key]['l'].append(t['light'])
        else:
          monthly[month_key] = {'r1':[t['w1_avg_ms']],'r2':[t['w2_avg_ms']],'r3':[t['w3_avg_ms']],\
                            't':[t['temp']],'l':[t['light']]}
      means = {}
      for month in monthly.keys():
        means[month] = {}
        monthly[month]['r1_mean'] = self.__calcmean__(monthly[month]['r1'])
        means[month]['r1'] = monthly[month]['r1_mean']
        monthly[month]['r2_mean'] = self.__calcmean__(monthly[month]['r2'])
        means[month]['r2'] = monthly[month]['r2_mean']
        monthly[month]['r3_mean'] = self.__calcmean__(monthly[month]['r3'])
        means[month]['r3'] = monthly[month]['r3_mean']
        monthly[month]['t_mean'] = self.__calcmean__(monthly[month]['t'])
        means[month]['t'] = monthly[month]['t_mean']
        monthly[month]['l_mean'] = self.__calcmean__(monthly[month]['l'])
        means[month]['l'] = monthly[month]['l_mean']
      return means

    if period == 'weekly':
      for entry in self.data_container[1:]:
        t = entry
        month_key = t['tstamp'].strftime("%Y-%m")
        if monthly.has_key(month_key):
          monthly[month_key]['r1'].append(t['w1_avg_ms'])
          monthly[month_key]['r2'].append(t['w2_avg_ms'])
          monthly[month_key]['r3'].append(t['w3_avg_ms'])
          monthly[month_key]['t'].append(t['temp'])
          monthly[month_key]['l'].append(t['light'])
        else:
          monthly[month_key] = {'r1':[t['w1_avg_ms']],'r2':[t['w2_avg_ms']],'r3':[t['w3_avg_ms']],\
                            't':[t['temp']],'l':[t['light']]}
      means = {}
      for month in monthly.keys():
        means[month] = {}
        monthly[month]['r1_mean'] = self.__calcmean__(monthly[month]['r1'])
        means[month]['r1'] = monthly[month]['r1_mean']
        monthly[month]['r2_mean'] = self.__calcmean__(monthly[month]['r2'])
        means[month]['r2'] = monthly[month]['r2_mean']
        monthly[month]['r3_mean'] = self.__calcmean__(monthly[month]['r3'])
        means[month]['r3'] = monthly[month]['r3_mean']
        monthly[month]['t_mean'] = self.__calcmean__(monthly[month]['t'])
        means[month]['t'] = monthly[month]['t_mean']
        monthly[month]['l_mean'] = self.__calcmean__(monthly[month]['l'])
        means[month]['l'] = monthly[month]['l_mean']
      return means

    if period == 'daily':
      for entry in self.data_container[1:]:
        t = entry
        day_key = t['tstamp'].strftime("%Y-%m-%d")
        if daily.has_key(day_key):
          daily[day_key]['r1'].append(t['w1_avg_ms'])
          daily[day_key]['r2'].append(t['w2_avg_ms'])
          daily[day_key]['r3'].append(t['w3_avg_ms'])
          daily[day_key]['t'].append(t['temp'])
          daily[day_key]['l'].append(t['light'])
        else:
          daily[day_key] = {'r1':[t['w1_avg_ms']],'r2':[t['w2_avg_ms']],'r3':[t['w3_avg_ms']],\
                            't':[t['temp']],'l':[t['light']]}
      means = {}
      for day in daily.keys():
        means[day] = {}
        daily[day]['r1_mean'] = self.__calcmean__(daily[day]['r1'])
        means[day]['r1'] = daily[day]['r1_mean']
        daily[day]['r2_mean'] = self.__calcmean__(daily[day]['r2'])
        means[day]['r2'] = daily[day]['r2_mean']
        daily[day]['r3_mean'] = self.__calcmean__(daily[day]['r3'])
        mean[day]['r3'] = daily[day]['r3_mean']
        daily[day]['t_mean'] = self.__calcmean__(daily[day]['t'])
        mean[day]['t'] = daily[day]['t_mean']
        daily[day]['l_mean'] = self.__calcmean__(daily[day]['l'])
        mean[day]['l'] = daily[day]['l_mean']
      return means
      

if __name__ == '__main__':
  from optparse import OptionParser
  oparser = OptionParser()
  (options, args) = oparser.parse_args()
  if len(args) != 1:
    oparser.error("You need to provide this script with exactly one argument - the DATA.txt file produced by the buttermill")
    sys.exit(1)
  else:
    parser = ButtermillParser(args[0])
    parser.parse()
    for r in parser.data_container[1:]: # we didn't parse the first item in the list (see above)
      print "%s\trotor 1: %.2f m/s\trotor 2: %.2f m/s\trotor 3: %.2f m/s\ttemp: %d  light: %s"%(datetime.strftime(r['tstamp'],"%Y-%m-%d %H:%M:%S"),r['w1_avg_ms'],r['w2_avg_ms'],r['w3_avg_ms'],r['temp'],r['light'])
