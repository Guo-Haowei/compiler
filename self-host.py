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
sh_exe_name = 'cc2'


def test_main():
    test.setup('self-host')

    # build compiler with cc
    build_with_cc = []
    for f in build.src_files:
        build_with_cc.append(f)

    cc_src = ''
    cc_asm = ''
    for f in build_with_cc:
        cc_src += f'../src/{f} '
        cc_asm += f'{os.path.basename(f.replace(".c", ".s"))} '

    safe_subprocess(f'./{build.exe_name} {include_flag} {cc_src}')

    safe_run(
        f'gcc {build.boundary} {cc_asm} -o {sh_exe_name}')
    safe_run(f'rm {build.exe_name}')

    testcases = test.get_test_list()
    for f in testcases:
        test.test_file(f, sh_exe_name)

    print('All tests passed')
    return


if __name__ == "__main__":
    test_main()
