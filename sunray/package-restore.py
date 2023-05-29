from os.path import join, isfile

Import("env")

FRAMEWORK_DIR = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
original_file =  "HardwareSerial.cpp"
blocked_file =  join(FRAMEWORK_DIR, "cores", "esp32", "HardwareSerial.cpp_not_now")
#assert not isfile(original_file) and isfile(blocked_file)
# rename package file before compile
env.Execute("rename %s %s" % (blocked_file, original_file))