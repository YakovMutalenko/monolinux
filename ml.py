import sys
import argparse
from subprocess import run
from pyinotify import WatchManager
from pyinotify import Notifier
from pyinotify import EventsCodes
from pyinotify import ProcessEvent


__version__ = '0.1.0'


def on_modify(s):
    print(80 * '=')
    run(['make', '-s', '-j4'])


def paths_to_watch():
    stdout = run(['make', '-s', 'print-SRC'],
                 capture_output=True,
                 encoding='utf-8').stdout

    return stdout.split()


def do_test(_):
    wm = WatchManager()
    notifier = Notifier(wm, default_proc_fun=ProcessEvent())

    for path in paths_to_watch():
        wm.add_watch(path, EventsCodes.ALL_FLAGS['IN_MODIFY'])

    notifier.loop(callback=on_modify)


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument('-d', '--debug', action='store_true')
    parser.add_argument('--version',
                        action='version',
                        version=__version__,
                        help='Print version information and exit.')

    # Workaround to make the subparser required in Python 3.
    subparsers = parser.add_subparsers(title='subcommands',
                                       dest='subcommand')
    subparsers.required = True

    # The 'test' subparser.
    subparser = subparsers.add_parser('test',
                                      description='Automatic test execution.')
    subparser.set_defaults(func=do_test)

    args = parser.parse_args()

    if args.debug:
        args.func(args)
    else:
        try:
            args.func(args)
        except BaseException as e:
            sys.exit('error: {}'.format(str(e)))

if __name__ == '__main__':
    main()
