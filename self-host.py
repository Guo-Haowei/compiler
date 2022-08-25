import os
import subprocess
import build
import test


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


include_flag = '-I ../include'


def test_file(file):
    print(f'running test {file}.c')

    # # generate .s
    # safe_subprocess(f'./{build.exe_name} {include_flag} ../test/{file}.c')
    # # compile
    # safe_run(f'gcc -c {file}.s -o {file}.o')
    # safe_run(f'gcc -o tmp assert_impl.o {file}.o')

    # child = subprocess.Popen('./tmp')
    # child.communicate()
    # retcode = child.poll()
    # if retcode != 0:
    #     print(f'    in file {test.test_src_folder}{file}.c(error: {retcode})')
    #     os._exit(1)


sh_exe_name = 'minic2'


def test_main():
    test.setup('self-host')
    # build compiler with minic
    build_with_minic = ['generic/array.c', 'generic/list.c']
    build_with_gcc = []
    for f in build.src_files:
        if not f in build_with_minic:
            build_with_gcc.append(f'../src/{f}')

    minic_src = ''
    minic_asm = ''
    for f in build_with_minic:
        minic_src += f'../src/{f} '
        minic_asm += f'{os.path.basename(f.replace(".c", ".s"))} '

    safe_subprocess(f'./{build.exe_name} {include_flag} {minic_src}')

    safe_run(f'gcc {minic_asm} {" ".join(build_with_gcc)} -o {sh_exe_name}')
    safe_run(f'rm {build.exe_name}')

    testcases = test.get_test_list()
    for f in testcases:
        test.test_file(f, sh_exe_name)

    return

    # compile assert_impl.c
    cmds = ['gcc', '-c', f'{test.test_src_folder}assert_impl.c']
    safe_run(cmds)

    blacklist = ['assert_impl.c', 'tmp.c']
    if len(cases) == 0:
        for f in os.listdir(f'{build.build_proj_path()}/test'):
            if f.endswith('.c'):
                if f not in blacklist:
                    size = len(f)
                    cases.append(f[:size - 2])

    # run tests
    for f in cases:
        test_file(f)

    print('All tests passed')
    return


if __name__ == "__main__":
    test_main()
