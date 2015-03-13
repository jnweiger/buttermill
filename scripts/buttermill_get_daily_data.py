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
#    * Neither the name of the <ORGANIZATION> nor the names of its contributors may be used to endorse or
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
# This script scrapes the met date from met eireann for a defined period of days
# In order to speed the script up (the script performs non-dependent url lookups), threading is used
import sys
import os
import re
import datetime
import urllib
import urllib2
import itertools
from threading import Thread
from optparse import OptionParser

SCRIPT = 'http://www.met.ie/climate/daily-data.asp'
METHOD = 'POST'
STATION = 'Shannon Airport'
DAYS = 30 # how many days to go back. max is 365
TODAY = datetime.datetime.today()
YESTERDAY = TODAY-datetime.timedelta(1)

parser = OptionParser()
parser.add_option("-f", "--format",
                  action="store", dest="format", default="csv",
                  help="Choose between csv and stdout")
parser.add_option("-o", "--output",
                  action="store", dest="output", default=None,
                  help="Specify an output file into which the csv data will be written")

(options, args) = parser.parse_args()


if len(args) == 1:
  x = int(args[0])
  if x != 0 and x+1 < 366:
    DAYS = x
  else:
    parser.error('%d is an invalid number of days to go back. Enter an integer between 1 and 364\n'%x)
    sys.exit(1)

if options.format:
  if options.format.lower() == 'csv':
    fmt = 'csv'
  elif options.format.lower() == 'stdout':
    fmt = 'stdout'
  else:
    fmt = 'stdout' # necessary?

if options.output:
  x = os.path.abspath(options.output)
  try:
    fd = open(x,'w')
  except IOError:
    fd.close()
    parser.error('No permission to write to output file %s'%options.output)
    sys.exit(1)
  else:
    fd.close()

def knot_to_ms(knot):
  return knot*0.5144

def date_generator():
  """
  generate a list of datetime objects starting with today and going back in time by DAYS
  """
  from_date = YESTERDAY
  while True:
    yield from_date
    from_date = from_date - datetime.timedelta(days=1)

def do_lookup(date_str,station):
  """
  date_str: datetime.datetime object
  station: string object, defaults to STATION
  returns: tuple containing date string (%d/%m/%Y) and float representing windspeed in m/s
  """
  day = date_str.day
  month = date_str.month
  year = date_str.year
  date_str = "%s/%s/%s"%(day,month,year)
  values = {'startdate':date_str, 'stationname':STATION}
  data = urllib.urlencode(values)
  req = urllib2.Request(SCRIPT, data)
  response = urllib2.urlopen(req)
  the_page = response.read().replace(" ","").replace("\t","").replace("\r\n","")
  x = re.findall(r'<td>%s</td>.*?</tr>'%date_str,the_page)
  target_tr = x[0].replace("<tr>","").split("</td>")
  data_list = []
  for x in target_tr:
    data_list.append(x.replace("<td>",""))
  date_idx = 0
  windspeed_idx = 6
  maxt_idx,mint_idx = 2,3
  dateobj = datetime.datetime.strptime(data_list[date_idx],"%d/%m/%Y")
  windspeed =  knot_to_ms(float(data_list[windspeed_idx]))
  maxt = float(data_list[maxt_idx])
  mint = float(data_list[mint_idx])
  return {'date':dateobj,'windspeed':windspeed,'maxt':maxt,'mint':mint}

dates_list  = list(itertools.islice(date_generator(), DAYS))
return_list = []

class testit(Thread):
   def __init__ (self,date_str,station):
      Thread.__init__(self)
      self.date_str = date_str
      self.station = station

   def run(self):
        scrape = do_lookup(self.date_str,self.station)
        return_list.append(scrape)

scrapelist = []

for d in dates_list:
   current = testit(d,STATION)
   scrapelist.append(current)
   current.start()

for s in scrapelist:
   s.join()

return_list.sort(key=lambda item:item['date'], reverse=True)

if fmt == 'stdout':
  for d in return_list:
    print "%s : %.2f m/s"%(d['date'].strftime("%Y-%m-%d"),d['windspeed'])
elif fmt == 'csv':
  fd = open(os.path.abspath(options.output),'w')
  for d in return_list:
    fd.write('%s,%s,%.2f,%.2f,%.2f\n'%(STATION,d['date'].strftime("%Y-%m-%d"),d['windspeed'],d['maxt'],d['mint']))
  fd.close()
