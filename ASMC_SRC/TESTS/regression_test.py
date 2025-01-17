import gzip
import os
import subprocess
import sys


def test_regession(asmc_exe):
    """
    Run the ASMC regression test, which will test the output of an example ASMC run with the cached result in
    data/regression_test_original.gz.

    :param asmc_exe: path to the ASMC executable
    """

    script_dir = os.path.realpath(os.path.dirname(__file__))
    base_dir = os.path.realpath(os.path.join(script_dir, '..', '..'))
    old_file = os.path.join(script_dir, 'data', 'regression_test_original.gz')
    assert os.path.isfile(old_file)

    # Old file contents are before OxfordRSE involvement in ASMC
    with gzip.open(old_file, 'rt') as gz_f:
        old_lines = gz_f.readlines()

    # New file contents are the result of running the example with the current ASMC source
    decoding_file = os.path.join(base_dir, 'FILES', 'DECODING_QUANTITIES', '30-100-2000.decodingQuantities.gz')
    haps_file = os.path.join(base_dir, 'FILES', 'EXAMPLE', 'exampleFile.n300.array')

    subprocess.call([
        asmc_exe,
        '--decodingQuantFile', decoding_file,
        '--hapsFileRoot', haps_file,
        '--posteriorSums',
    ])

    new_file = os.path.join(base_dir, 'FILES', 'EXAMPLE', 'exampleFile.n300.array.1-1.sumOverPairs.gz')
    assert os.path.isfile(new_file), \
        "No output file found at {}. Did the executable run as expected?".format(new_file)

    with gzip.open(new_file, 'rt') as gz_f:
        new_lines = gz_f.readlines()

    assert len(old_lines) == len(new_lines), \
        "The outputs have different numbers of lines ({} and {})".format(len(old_lines), len(new_lines))

    for i, (old, new) in enumerate(zip(old_lines, new_lines)):
        assert old == new, "The outputs first differ at line {}".format(i)

    print('\n' + '#' * 35)
    print('#      Regression test passed     #')
    print('# All {} output lines identical #'.format(len(old_lines)))
    print('#' * 35 + '\n')


if __name__ == "__main__":
    assert len(sys.argv) == 2, "Usage: {} /path/to/ASMC_exe".format(sys.argv[0])

    path_to_asmc = sys.argv[1]
    assert os.path.isfile(path_to_asmc) and 'ASMC_exe' in path_to_asmc, \
        "Expected path to ASMC executable, but got {}".format(path_to_asmc)

    test_regession(path_to_asmc)
