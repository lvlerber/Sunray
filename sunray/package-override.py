#from ast import Import

from os.path import join, isfile

Import("env")

FRAMEWORK_DIR = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
original_file = join(FRAMEWORK_DIR, "cores", "esp32", "HardwareSerial.cpp")
blocked_file_name =   "HardwareSerial.cpp_not_now"
blocked_file = join(FRAMEWORK_DIR, "cores", "esp32", blocked_file_name)
print('Renaming  ' + original_file + ' to ' + blocked_file + '\n')
print(f'isfile(original_file) {isfile(original_file)} \n')
print(f'isfile(blocked_file) {isfile(blocked_file)}  \n')
assert (isfile(original_file) and not isfile(blocked_file)) or (not isfile(original_file) and isfile(blocked_file))
# rename package file before compile
# in case a buld failed, the post is not executed and the original file remains renamed.
if isfile(original_file):
    env.Execute("rename %s %s" % (original_file, blocked_file_name))