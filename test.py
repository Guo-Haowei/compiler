import os
import subprocess
import build
import sys

test_src_folder = f'{build.build_proj_path()}/test/'
checkmark = u'\u2713'
crossmark = u'\u2717'

rules = {
    'unit.util': {
        'defines': [],
        'extra': ['src/generic/list.c', 'src/utility.c']
    },
    'unit.array': {
        'defines': [],
        'extra': ['src/generic/array.c']
    },
    'unit.list': {
        'defines': [],
        'extra': ['src/generic/list.c']
    },
    'extern': {
        'defines': [],
        'extra': ['test/extern2.c']
    }
}


def safe_subprocess(cmd):
    child = subprocess.Popen(cmd)
    child.communicate()
    if child.poll() != 0:
        print(f'Failed to execute command: {cmd}')
        os._exit(1)


def safe_run(cmdList):
    cmd = cmdList
    if not isinstance(cmdList, str):
        cmd = ' '.join(cmdList)
    if os.system(cmd) != 0:
        print(f'Failed to execute command: {cmd}')
        os._exit(1)
    return


def test_file(file, compiler):
    print(f'running test {file}.c')
    include_flag = '-I ../include'
    if file in rules:
        defines = rules[file]['defines']
        srcs = [f'../test/{file}.c']
        for f in rules[file]['extra']:
            srcs.append(f'../{f}')
        # generate .s
        safe_subprocess(f'./{compiler} {include_flag} {" ".join(srcs)}')

        asms = []
        for f in srcs:
            name = os.path.basename(f)
            asms.append(name.replace('.c', '.s'))
        safe_run(f'gcc {" ".join(asms)} -o tmp')
    else:
        # generate .s
        safe_subprocess(f'./{compiler} {include_flag} ../test/{file}.c')
        # compile
        safe_run(f'gcc {file}.s -o tmp')

    child = subprocess.Popen('./tmp')
    child.communicate()
    retcode = child.poll()
    if retcode != 0:
        print(f'    in file {test_src_folder}{file}.c(error: {retcode})')
        os._exit(1)


def setup(folder):
    # create tmp folder
    if os.path.exists(folder):
        os.system(f'rm -r {folder}')

    try:
        os.mkdir(folder)
    except:
        print(f'failed to create folder {folder}')
        os._exit(1)

    # change working directory
    os.chdir(folder)
    build.build_exe()


def get_test_list():
    cases = []
    blacklist = ['tmp.c', 'extern2.c']
    for f in os.listdir(f'{build.build_proj_path()}/test'):
        if f.endswith('.c') and f not in blacklist:
            size = len(f)
            cases.append(f[:size - 2])
    return cases


def test_main(cases):
    setup('tmp')

    if len(cases) == 0:
        cases = get_test_list()

    # run tests
    for f in cases:
        test_file(f, build.exe_name)

    print('All tests passed')
    return


if __name__ == "__main__":
    test_main(sys.argv[1:])
