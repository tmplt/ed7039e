from setuptools import setup
from os import listdir
from os.path import isfile

def is_lcm_script(f):
    return (f.startswith("lcm-")
            and f.endswith(".py")
            and isfile(f))

setup(scripts = [ f for f in listdir("./") if is_lcm_script(f) ])
