import collections
import os
import subprocess
import sys

root_dir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))

black_file_list = [
    os.path.join(root_dir, 'src', 'net', 'net_error_list.h')
]

def execute(argv):
    try:
        output = subprocess.check_output(argv, stderr=subprocess.STDOUT)
        return True, output
    except subprocess.CalledProcessError as e:
        return False, e.output

def find_files():
    src = os.path.join(root_dir, 'src')
    found_files = []
    for dirpath, dirname, files in os.walk(src):
        for name in files:
            file_name, file_extension = os.path.splitext(name)
            if file_extension not in ['.cc', '.h', '.cpp', '.cu', '.cuh']:
                continue

            f = os.path.join(dirpath, name)
            if f not in black_file_list:
                found_files.append(f)

    return found_files

def run_cpplint(files):
    cpplint = os.path.join(root_dir, 'script', 'cpplint.py')
    return execute([sys.executable, cpplint, '--root=src'] + files)

def main():
    success, output = run_cpplint(find_files());
    if success:
        if '-v' in sys.argv:
            print output
    else:
        sys.exit(output)

if __name__ == '__main__':
    sys.exit(main())
