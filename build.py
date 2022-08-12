import os

exe_name = 'minic'


def build_proj_path():
    proj_path = os.path.abspath(__file__)
    proj_path = proj_path.replace('\\', '/')
    proj_path = proj_path.split('/')
    proj_path.pop()
    proj_path = '/'.join(proj_path)
    return proj_path


def build_exe():

    cmd = ['gcc']

    src_files = [
        'generic/array.c',
        'generic/list.c',
        'file_cache.c',
        'codegen.c',
        'lexer.c',
        'main.c',
        'parser.c',
        'preproc.c',
        'misc.c',
        'string.c',
        'type.c',
    ]

    flags = [
        '-Wall',
        '-Wextra',
        '-Wno-unused-local-typedefs',
        '-mpreferred-stack-boundary=3'  # stack aligned to 8
    ]

    cmd = cmd + flags + ['-o', exe_name]
    for f in src_files:
        cmd.append(f'{build_proj_path()}/src/{f}')

    if os.system(' '.join(cmd)) != 0:
        print(f'Failed to compile {exe_name}')
        return

    return


if __name__ == "__main__":
    build_exe()
