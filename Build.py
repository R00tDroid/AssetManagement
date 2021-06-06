import os
import shutil
import sys
import subprocess

if not os.path.isdir('.build'):
	os.mkdir('.build')

orig_stdout = sys.stdout
f = open('.build\\autobuild.log', 'w')
sys.stdout = f

versions = ["5.0ea", "4.26", "4.24", "4.22", "4.20", "4.18"]

versions.sort(reverse = True)

root = os.path.dirname(os.path.realpath(__file__))

install_dir = os.environ.get('UE4_Install')

if install_dir is None:
	print("UE4_Install is not setup correctly")
	exit(-1)
	
out_dir = root + "\\BuildOutput"
if os.path.isdir(out_dir):
	print("Cleaning output at: " + out_dir)
	shutil.rmtree(out_dir)

res_success = 0
res_error = 0
	
for version in versions:
	print("Building version: " + version)
	
	cmd = "call \"" + install_dir + "\\UE_" + version + "\\Engine\\Build\\BatchFiles\\RunUAT.bat\" BuildPlugin -Plugin=\"" +  root + "\Plugins\AssetManagement\AssetManagement.uplugin\" -Package=\"" + out_dir + "\AssetManagement_" + version + "\" -Rocket"
	result = os.system(cmd)
	
	if result is not 0:
		res_error+=1
		dir = out_dir + "\\AssetManagement_" + version
		if os.path.isdir(dir):
			shutil.rmtree(dir)
	else:
		res_success+=1
		
print("Finished building! Success: ", res_success, ", Failure: ", res_error)

sys.stdout = orig_stdout
f.close()

input("Press Enter to exit...")