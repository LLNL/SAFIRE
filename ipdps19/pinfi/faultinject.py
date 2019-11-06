#!/usr/bin/env python3

import sys
import os
import getopt
import time
import random
import signal
import subprocess

pindir = os.environ['PINDIR']
currdir = os.getcwd()
pinbin = pindir + 'pin'
if sys.platform == 'darwin':
  LIBEXT = '.dylib'
else:
  LIBEXT = '.so'

instcategorylib = pindir + "source/tools/pinfiv2/obj-intel64/instcategory" + LIBEXT
instcountlib = pindir +"source/tools/pinfiv2/obj-intel64/instcount" + LIBEXT
filib = pindir + "source/tools/pinfiv2/obj-intel64/faultinjection" + LIBEXT
outputdir = currdir + "/prog_output"
basedir = currdir + "/baseline"
errordir = currdir + "/error_output"
statsdir = currdir + "/stats"

if not os.path.isdir(outputdir):
  os.mkdir(outputdir)
if not os.path.isdir(basedir):
  os.mkdir(basedir)
if not os.path.isdir(errordir):
  os.mkdir(errordir)
if not os.path.isdir(statsdir):
  os.mkdir(statsdir)

timeout = 1800

def execute( execlist):
	#print "Begin"
	#inputFile = open(inputfile, "r")
  global outputfile
  outputFile = open(outputfile, "w")
  #print outputfile
  print('execlist:' + ' '.join(execlist)) # ggout
  p = subprocess.Popen(execlist, stdout = outputFile)
  elapsetime = 0
  while (elapsetime < timeout):
    elapsetime += 1
    time.sleep(1)
    #print p.poll()
    if p.poll() is not None:
      print("\t program finish", p.returncode)
      print("\t time taken", elapsetime)
      #outputFile = open(outputfile, "w")
      #outputFile.write(p.communicate()[0])
      outputFile.close()
      #inputFile.close()
      return str(p.returncode)
  #inputFile.close()
  outputFile.close()
  print("\tParent : Child timed out. Cleaning up ... ")
  p.kill()
  return "timed-out"
	#should never go here
  sys.exit(syscode)


def main():
  #clear previous output
  global start_trial, end_trial, progargs, outputfile
  outputfile = basedir + "/golden_output"
  execlist = [pinbin, '-t', instcategorylib, '--', progbin]
  execlist.extend(progargs)
  execute(execlist)

  # baseline
  instcount_file = 'pin.instcount-' + str(start_trial) + '-' + str(end_trial)
  outputfile = basedir + "/golden_output"
  execlist = [pinbin, '-t', instcountlib, '-o', instcount_file, '--', progbin]
  execlist.extend(progargs)
  execute(execlist)
  # fault injection
  for index in range(start_trial, end_trial+1):
    outputfile = outputdir + "/outputfile-" + str(index)
    errorfile = errordir + "/errorfile-" + str(index)
    fioutputfile = statsdir +'/pin.injection-' + str(index)
    fiinstrfile = statsdir + '/pin.instr-' + str(index)
    execlist = [pinbin, '-t', filib, '-o', instcount_file,
            '-fi-output', fioutputfile,
            '-fi-instr', fiinstrfile,
            '--', progbin]
    execlist.extend(progargs)
    error_File = open(errorfile, 'w')
    error_File.close()
    ret = execute(execlist)
    if ret == "timed-out":
      error_File = open(errorfile, 'w')
      error_File.write("Program hang\n")
      error_File.close()
    elif int(ret) < 0:
      error_File = open(errorfile, 'w')
      error_File.write("Program crashed, terminated by the system, return code " + ret + '\n')
      error_File.close()
    elif int(ret) > 0:
      error_File = open(errorfile, 'w')
      error_File.write("Program crashed, terminated by itself, return code " + ret + '\n')
      error_File.close()

if __name__=="__main__":
  global progbin, start_trial, end_trial, progargs
  if len(sys.argv) < 4:
      print('Format: faultinject.py <prog> <# start trial> <# end trial> <input>')
      sys.exit(1)
  progbin = sys.argv[1]
  start_trial = int(sys.argv[2])
  end_trial = int(sys.argv[3])
  progargs = sys.argv[4:]
  main()
