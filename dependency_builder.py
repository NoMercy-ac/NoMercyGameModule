#!/usr/local/bin/python2.7

import subprocess
import shutil
import getopt
import sys
import os
import re

def build_tree(dir):
	abs = os.path.dirname(os.path.abspath(dir))
	os.popen("chmod -R 777 "+abs)
	
	if not os.path.exists(abs):
		if not build_tree(abs):
			return False
	
	try:
		if not os.path.exists(dir):
			os.mkdir(dir)
	except:
		return False
	
	return True

def extract_dependencies(src):
	return re.findall("(/\S*(?<!:))", subprocess.Popen(["ldd", src], stdout=subprocess.PIPE).communicate()[0])

def export_lib(outdir, infiles):
	pathmap = []
	
	# print("outdir: {0}".format(outdir))
	
	for infile in infiles:
		print("infile: {0}".format(infile))
		if not os.path.exists(infile):
			print("file not found: {0}".format(infile))
			continue

		print("extracting dependencies ({0})".format(infile))
  
		deps = extract_dependencies(infile)
		for dep in deps:
			if dep == infile:
				continue
			
			real_dep = os.path.realpath(dep)
			print("\tdep: {0} -> {1}".format(dep, real_dep))
			
			if pathmap.count(real_dep) == 0:
				pathmap.append(real_dep)
	
	for src in pathmap:
		if not os.path.isfile(src):
			return 2
		
		dst = os.path.dirname(outdir + src)
		print("building directory tree for {0}".format(dst))
		
		if not build_tree(dst):
			return 3
		
		try:
			print("copy '{0}' to '{1}'...".format(src, dst))
			#shutil.copyfile(src, dst)
			os.popen("cp -R "+src+" "+dst)
			os.popen("chmod -R 777 "+dst)
		except Exception as e:
			print(e)
			return 4
	
	return 0

def main(argv):
	outdir = "libexport"
	infiles = []

	try:
		opts, args = getopt.getopt(argv, "hi:o:", ["in=", "out="])
	except getopt.GetoptError:
		print("dependency_builder.py -o outputdirectory -i inputfile1 -i inputfile2")
		return 1
	
	for opt, arg in opts:
		if opt == "-h":
			print("dependency_builder.py -o outputdirectory -i inputfile1 -i inputfile2")
		elif opt in ("-i", "--in"):
			infiles.append(os.path.abspath(arg))
		elif opt in ("-o", "--out"):
			outdir = arg
	
	return export_lib(os.path.abspath(outdir), infiles)

if __name__ == "__main__":
	sys.exit(main(sys.argv[1:]))
