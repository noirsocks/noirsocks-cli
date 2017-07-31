import os

project_include_paths = ['./core/include', './core/src']

def GetIncludes():
    base_name = os.path.split(os.path.realpath(__file__))[0]
    return ['-I' + base_name + '/' + i for i in project_include_paths]

def FlagsForFile( filename, **kwargs ):
    flags = ['-x', 'c++', '--std=c++11'] + GetIncludes()
    return {'flags': flags}