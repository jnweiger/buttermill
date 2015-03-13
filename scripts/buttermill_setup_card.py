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
# This script setups up a buttermill configuration file (CONF.TXT) in the appropriate directory. On openSUSE
# or any system that mounts removable media to /media/<LABEL>/ the script should work as is, providing that
# the label of the removable SD card is BUTTERMILL. You can otherwise specify the file that should be written
# (note that CONF.TXT is hardcoded in main.c of the buttermill firmware) by adding the full path to the file
# as the first and only argument to this script

import sys,os,time
from optparse import OptionParser
DEFAULT_LOG_INTERVAL = '300' # as string - it gets converted
parser = OptionParser()
parser.add_option("-l", "--log-interval",
                  action="store", dest="log_interval", default=DEFAULT_LOG_INTERVAL,
                  help="Define how many seconds between each logging to the memory card")
(options, args) = parser.parse_args()

# nasty hack to get the logging interval to be 10 digits long
if not options.log_interval.isdigit():
  parser.error('The log interval must be a digit between 1 and 86400 (1 day in seconds)\n')
  sys.exit(1)
else:
  raw_logging_int = int(options.log_interval)
  if raw_logging_int < 1 or raw_logging_int > 86400:
    parser.error('The log interval must be a digit between 1 and 86400 (1 day in seconds)\n')
    sys.exit(1)
  else: 
    post_str = ""
    as_str = str(raw_logging_int)
    i = 0
    while len(post_str) < 10:
      post_str = "%s%s"%("0"*i,as_str)
      i+=1

if len(args) == 1:
  x = os.path.abspath(args[0])
  try:
    fd = open(x,'w')
  except IOError:
    fd.close()
    sys.stderr.write("No permission to write to file %s\n"%x)
  else:
    fd.close()
    WRITE_FILE = os.path.abspath(x)
else:
  MOUNTPOINT = '/media/'
  CARDLABEL = 'BUTTERMILL'
  CONFFILE = 'CONF.TXT'
  WRITE_DIR = os.path.abspath(os.path.join(MOUNTPOINT,CARDLABEL))
  if not os.path.isdir(WRITE_DIR):
    sys.stderr.write("The SD card root directory %s does not exist\n"%WRITE_DIR)
    sys.exit(1)
  else:
    if not os.access(WRITE_DIR,os.W_OK):
      sys.stderr.write("No permission to write to SD card root directory %s\n"%WRITE_DIR)
      sys.exit(1)
    else:
      WRITE_FILE = os.path.abspath(os.path.join(WRITE_DIR,CONFFILE))

try:
  fd = open(WRITE_FILE,'w')
except IOError:
  sys.stderr.write("No permission to open %s for writing\n"%WRITE_FILE)
  sys.exit(1)
else:
  # nasty hack to get the logging interval to be 10 digits long
  raw_logging_int = options.log_interval
  if not isinstance(raw_logging_int,int):
    sys
  NOW = int(time.time())
  fd.write("t=%u\n"%NOW)
  fd.write("l=%s\n"%post_str)
  fd.close()
  print "Wrote t=%u and l=%s to the file %s"%(NOW,post_str,WRITE_FILE)
  sys.exit(0)
