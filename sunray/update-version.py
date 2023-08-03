from datetime import datetime
version_file = "version.h"
file = open(version_file, mode='w')
file.write('#define VERSION "Version compiled at %s"' %datetime.now()) 
file.close()
