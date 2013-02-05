#!/Applications/py

import os
import wconsole
import wregistry
import sys

reg = wregistry.Registry()
currPath = "/"

def RegListSet(args):
	global currPath
	path = None

	if len(args) >= 1:
		path = args[0]
		
	if path == None:
		path = currPath
		
	currSet = reg.openKeySet(path)
	setIter = currSet.getSetIter()
	for name in setIter:
		type = currSet.getEntryType(name)
		if type == wregistry.EntryKeySet:
			wconsole.setForeColor(wconsole.ColorBlue)

		sys.stdout.write(name)

		if type == wregistry.EntryKeySet:
			print "/"
			wconsole.colorReset()
		else:
			sys.stdout.write("\n")
			
	return False

def RegChangeSet(args):
	global currPath, reg
	if len(args) == 0:
		return False

	path = os.path.join(currPath, args[0])
	path = os.path.normpath(path)

	try:
		nextSet = reg.openKeySet(path)
		currPath = path
	except:
		print "Could not find set with name %s" % path

def RegExit(args):
	return True
	
commandList = { "ls" : RegListSet, "list" : RegListSet, "dir" : RegListSet,
			"cd" : RegChangeSet, "chdir" : RegChangeSet,
			"exit" : RegExit }

def RegProcessLine(line):
	line = line.strip("\n")
	tokens = line.split(" ")
	command = tokens[0]
	
	if command in commandList.keys():
		f = commandList[command]
		quit = f(tokens[1:])
	else:
		print "Invalid command", command
		quit = False
		
	return quit

def main():
	print "textreg v0.1"
	quit = False	
	
	while not quit:
		line = wconsole.readLine(currPath + ">")
		quit = RegProcessLine(line)
		
if __name__ == "__main__":
	sys.exit(main())
