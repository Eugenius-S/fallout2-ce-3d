"""Refuses evidence files in runs/ over the size limit (079).

Usage: python tools/check_runs_size.py <base ref>
Checks the files under runs/ that the branch adds or changes against the
base. A task's evidence is a JPEG sheet or a few cropped shots, not raw
1920x1080 PNG captures: those stay in game/shots/.
"""
import subprocess
import sys

LIMIT = 1024 * 1024


def git(*args):
    return subprocess.run(['git', *args], check=True, capture_output=True, text=True).stdout


def main():
    base = sys.argv[1]
    names = git('diff', '--name-only', '--diff-filter=AM', f'{base}...HEAD', '--', 'runs/').split('\n')
    over = []
    for name in filter(None, names):
        size = int(git('cat-file', '-s', f'HEAD:{name}'))
        if size > LIMIT:
            over.append((size, name))
    for size, name in over:
        print(f'{name}: {size / 1048576:.1f} MB, over the 1 MB limit')
    if over:
        print('Shrink it (a JPEG sheet, a crop, a smaller scale) or keep it in game/shots/.')
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
