# Bristlemouth HIL Test Scripts

The following directory contains hardware-in-the-loop (HIL) test scripts that utilize [pytest](https://docs.pytest.org/en/stable/).
This includes specific tests,
as well as helper files to run the tests.

## Setup

In order to run HIL testing,
[conda](https://docs.conda.io/en/latest/) is used to manage dependencies.
The following will create a conda environment and download/install all the dependencies:

```
$ conda create -n bm_hil
$ conda activate bm_hil
$ conda env update -f environment.yml
```

Once you're done, you can always deactivate the environment with:

```
$ conda deactivate
```

Whenever working with HIL testing ensure to activate the environment with

```
$ conda activate bm_hil
```

## Running HIL Tests

HIL tests require a serial communication to the device being tested.
The requirements for this serial interface are as so:

- Byte Size = 8 bits
- Parity = None
- Stop Bits = 1
- Flow Control = False

A specified baud rate is not a requirement.

In order to run the tests the following command must be invoked:

`pytest -s @tests_to_run.txt --port '{port}' --baud {baud}`

Where port is the serial port you are attempting to talk to the Bristlemouth node on,
and baud is the desired baud rate.

## Contributing

The code in all tests shall follow the [PEP 8](https://peps.python.org/pep-0008/) style guide for python.
This is enforced with [black](https://github.com/psf/black) which is installed with the conda environment.
Black can be ran after activating the conda environment and invoking:

`black path/to/test/scripts`

All function arguments shall be type hinted as in accordance to [PEP 484](https://peps.python.org/pep-0484/).
Docstrings shall be in the following format:

```python
"""Brief description of class/function

Detailed description of class/function

Args:
    param_1 (type_1): Description here.
    param_2 (type_2): Description here.
    param_3 (type_3): Description here.

Returns:
    {type}: Description of returned value.
"""
```

When adding new tests,
ensure the `tests_to_run.txt` file is updated.
The format of entries in this file is as so:

`test_script.py::ClassName::test_function_name`
