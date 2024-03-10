#!/usr/bin/env python2.7

from multiprocessing import Pool
import os
import requests
import sys
import time

# Globals

PROCESSES = 1
REQUESTS  = 1
VERBOSE   = False
URL       = None
SUCCESS   = 0
FAILURE   = 1

# Functions

def usage(status=0):
    print('''Usage: {} [-p PROCESSES -r REQUESTS -v] URL
    -h              Display help message
    -v              Display verbose output

    -p  PROCESSES   Number of processes to utilize (1)
    -r  REQUESTS    Number of requests per process (1)
    '''.format(os.path.basename(sys.argv[0])))
    sys.exit(status)



# Main execution
def parse_cli_args():
    """ Pareses the command line arguments and sets variables appropriately
    """
    global PROCESSES, VERBOSE, REQUESTS, URL
    argv = sys.argv
    URL = argv[-1]
    if '-h' in argv:
        usage(SUCCESS)
    if '-v' in argv:
        VERBOSE = True
    if '-p' in argv:
        p_idx = argv.index('-p')
        if p_idx >= len(argv):
            usage(FAILURE)
        try:
            PROCESSES = int(argv[p_idx+1])
        except NameError as e:
            print(f'Illegal value for process number: {e}')
            sys.exit(FAILURE)
        except ValueError as e:
            print(f'An error occurred: {e}')
            sys.exit(FAILURE)
    if '-r' in argv:
        r_idx = argv.index('-r')
        if r_idx >= len(argv):
            usage(FAILURE)
        try:
            REQUESTS = int(argv[r_idx + 1])
        except NameError as e:
            print("Illegal value for request number: {e}")
            sys.exit(FAILURE)
        except ValueError as e:
            print('An error occured: {e}')
            sys.exit(FAILURE)            


    
if __name__ == '__main__':


# vim: set sts=4 sw=4 ts=8 expandtab ft=python:
